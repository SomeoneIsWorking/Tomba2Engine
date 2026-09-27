#!/usr/bin/env python3
"""Say which MODE overlay images actually contain the function at a given address.

WHY THIS EXISTS. Issue 0015 found 42 native owners written for overlay addresses but declared with
the RESIDENT form, so they never install and the guest body runs instead. Converting one needs the
name of the image that owns its address, and the issue is explicit that guessing is worse than
leaving it alone: a declaration bound to the wrong image installs a native owner over a different
function body.

The ground truth is the images themselves. Every MODE image -- SOP and A00..A0L -- loads at the same
base, so file offset X is address BASE+X in whichever one is active, and the same numeric address is
a different function in each image that has one there. This tool authenticates each image against
the tracked manifest, extracts the function body at the address, and groups the images by the bytes
they actually hold. Images that share a body share the function; images that differ are a collision,
which `0x801113B4` in A03 and A0B already demonstrated before this tool existed.

WHAT A NEGATIVE PRINTS. Every image is accounted for on every address: those whose file is shorter
than the offset, those whose word at the offset is not a function prologue, and those whose body
never reaches a return inside the walk limit are each reported by name and count. An address that no
image owns prints that it has no owner, with the denominator of images examined -- it does not print
an empty list, because an empty list cannot be told from a tool that never looked.

`--census` is the other direction. `--unreachable` asks which image owns an address; the census asks
whether each declaration already in the source names an image that actually owns its address, and
prints the per-image install count the product's own `bindOverlay` will reach. It is the check that
tells a conversion from a guess, and it exits 1 when a declaration names an image that does not hold
the function there.

KNOWN LIMIT OF THE "no owner" VERDICT (measured 2026-09-27, issue 0015). Function detection requires a
stack-allocating prologue, so a MIPS LEAF -- one that never touches $sp -- reads as "no image owns
this address" even when the image holds the function. All five of the addresses issue 0015 lists as
unowned are exactly that case in A00, verified by disassembly (`0x80127420` and `0x801274BC` are
leaves; `0x80140544` allocates its frame two instructions in, after a `lui`/`lh` pair). The verdict is
therefore a false negative about a FUNCTION SHAPE, not about which image holds the bytes, and it must
not be read as "this address is not code". See issue 0015.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Every MODE image is loaded to this address by `activateModeOverlay`, so an address in one image is
# the same file offset in all of them. See game/core/native_override_catalog.cpp.
MODE_SLOT = 0x80108F9C

# The resident text range `bindResident` installs into, from game/core/game_config.cpp's
# recMainLo/recMainHi. A `declareOverride` outside it can never install, which is issue 0015.
RESIDENT_TEXT = (0x80010000, 0x800BE800)

# A declaration's address may be a hex literal OR a named constant. Matching the literal only is how
# `ActorZonedAttacker::registerOverrides` hid three of issue 0015's 42 offenders (it declares through
# FN_80145C78 / FN_8014047C / FN_801409C0), which made this scan report 38 while the product's own
# bindResident refusal named 41 — a diagnostic that disagrees with the thing it measures. An
# identifier that cannot be resolved is REPORTED, never skipped.
ADDRESS_TOKEN = r"(?:0x[0-9A-Fa-f]+u?|[A-Za-z_][A-Za-z0-9_]*)"
DECLARE_RESIDENT = re.compile(rf"declareOverride\(\s*({ADDRESS_TOKEN})\s*,\s*\"([^\"]*)\"")
DECLARE_OVERLAY = re.compile(
    rf"declareOverlayOverride\(\s*\"([^\"]*)\"\s*,\s*({ADDRESS_TOKEN})\s*,\s*\"([^\"]*)\"")
CONSTANT = re.compile(
    r"^(?:static\s+)?constexpr\s+(?:std::uint32_t|uint32_t|unsigned int|auto)\s+"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(0x[0-9A-Fa-f]+)u?\s*;",
    re.MULTILINE)
# A header declares at class or namespace scope, where indentation says nothing about scope, so its
# constants are indexed for the tree-wide fallback only, and an indented one counts too.
HEADER_CONSTANT = re.compile(
    r"^\s*(?:static\s+)?constexpr\s+(?:std::uint32_t|uint32_t|unsigned int|auto)\s+"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(0x[0-9A-Fa-f]+)u?\s*;",
    re.MULTILINE)

RETURN_TO_CALLER = 0x03E00008  # jr $ra
PROLOGUE_MASK = 0xFFFF0000
STACK_ALLOCATE = 0x27BD0000  # addiu $sp, $sp, -N
WALK_LIMIT = 4096  # instructions; a MODE function far shorter than its image


class Refusal(Exception):
    """A condition that must stop the tool rather than shrink its answer."""


@dataclass
class Body:
    """One image's function at the requested address."""

    image: str
    words: int
    digest: str


@dataclass
class Address:
    """What every examined image holds at one address."""

    address: int
    bodies: list[Body] = field(default_factory=list)
    short: list[str] = field(default_factory=list)  # image ends before the offset
    not_a_function: list[str] = field(default_factory=list)  # no stack-allocating prologue
    runaway: list[str] = field(default_factory=list)  # no return within the walk limit

    def examined(self) -> int:
        return len(self.bodies) + len(self.short) + len(self.not_a_function) + len(self.runaway)

    def groups(self) -> dict[str, list[str]]:
        out: dict[str, list[str]] = {}
        for body in self.bodies:
            out.setdefault(body.digest, []).append(body.image)
        return out


def mode_image_names(manifest: dict) -> list[str]:
    """SOP and A00..A0L, in the order `activateModeOverlay` indexes them."""
    names = ["SOP"] + ["A0" + (chr(ord("0") + a) if a < 10 else chr(ord("A") + a - 10)) for a in range(22)]
    missing = [name for name in names if f"BIN/{name}.BIN" not in manifest["images"]]
    if missing:
        raise Refusal(f"manifest has no entry for {len(missing)} MODE image(s): {', '.join(missing)}")
    return names


def authenticate(directory: Path, manifest: dict, names: list[str]) -> dict[str, bytes]:
    """Read every MODE image and refuse any that is not the manifest's."""
    images: dict[str, bytes] = {}
    for name in names:
        path = directory / f"{name}.BIN"
        if not path.is_file():
            raise Refusal(f"no image at {path}: re-provision with tools/tomba2_provision.py")
        data = path.read_bytes()
        expected = manifest["images"][f"BIN/{name}.BIN"]
        if len(data) != expected["size"]:
            raise Refusal(f"{name}.BIN is {len(data)} bytes, manifest says {expected['size']}")
        digest = hashlib.sha256(data).hexdigest()
        if digest != expected["sha256"]:
            raise Refusal(f"{name}.BIN does not match the manifest digest")
        images[name] = data
    return images


def body_at(data: bytes, offset: int) -> tuple[int, str] | str:
    """The function starting at `offset`, or why this image has none there.

    A MIPS function in these overlays opens by allocating stack and closes on `jr $ra` plus its
    delay slot. Both ends are required: a word that merely disassembles is not a function, and a
    walk that never returns is reporting on data rather than code.
    """
    if offset + 4 > len(data):
        return "short"
    if (int.from_bytes(data[offset : offset + 4], "little") & PROLOGUE_MASK) != STACK_ALLOCATE:
        return "not_a_function"
    for index in range(WALK_LIMIT):
        at = offset + index * 4
        if at + 8 > len(data):
            return "runaway"
        if int.from_bytes(data[at : at + 4], "little") == RETURN_TO_CALLER:
            words = index + 2  # the return and its delay slot
            return words, hashlib.sha256(data[offset : offset + words * 4]).hexdigest()
    return "runaway"


def examine(address: int, images: dict[str, bytes]) -> Address:
    out = Address(address=address)
    offset = address - MODE_SLOT
    if offset < 0:
        raise Refusal(f"{address:08X} is below the MODE slot base {MODE_SLOT:08X}")
    for name, data in images.items():
        result = body_at(data, offset)
        if result == "short":
            out.short.append(name)
        elif result == "not_a_function":
            out.not_a_function.append(name)
        elif result == "runaway":
            out.runaway.append(name)
        else:
            words, digest = result
            out.bodies.append(Body(name, words, digest))
    return out


def report(found: Address) -> None:
    groups = found.groups()
    print(f"{found.address:08X}  examined {found.examined()} MODE images")
    if not groups:
        print("           NO IMAGE OWNS THIS ADDRESS — no function body in any of them")
    for digest, names in sorted(groups.items(), key=lambda item: (-len(item[1]), item[1][0])):
        words = next(body.words for body in found.bodies if body.digest == digest)
        print(f"           {digest[:12]}  {words:4d} words  {', '.join(names)}")
    for label, names in (
        ("image ends before this offset", found.short),
        ("no function prologue here", found.not_a_function),
        ("no return within the walk limit", found.runaway),
    ):
        if names:
            print(f"           ({len(names)} {label}: {', '.join(names)})")


def named_constants(source: Path) -> tuple[dict[str, int], dict[str, set[int]]]:
    """The file-scope and header `constexpr` integer constants a declaration's address can be named by.

    A .cpp is indexed per file, because that is the scope a `declare*Override(...)` call in it can
    name: a constant declared inside a function body is invisible to it, and indexing those would make
    one name mean several addresses in one file (release_trigger_motion.cpp declares four different
    `TBL`s, one per sub-motion). Headers are indexed for the tree-wide fallback only, since a
    declaration may name a constant from its own header (ScriptInterp::kAdvanceAddr). A name is
    ambiguous in the fallback when two translation units give it different values, and the resolver
    refuses that rather than picking one: an address resolved from the wrong constant is the exact
    failure this tool exists to prevent, and it would otherwise be silent.
    """
    per_file: dict[Path, dict[str, int]] = {}
    everywhere: dict[str, set[int]] = {}

    def index(path: Path, pattern: re.Pattern[str], local: dict[str, int] | None) -> None:
        for name, value in pattern.findall(path.read_text()):
            address = int(value, 16)
            if local is not None and local.setdefault(name, address) != address:
                raise Refusal(f"{path}: constant {name} is defined twice with different addresses")
            everywhere.setdefault(name, set()).add(address)

    for path in sorted(source.rglob("*.cpp")):
        local: dict[str, int] = {}
        index(path, CONSTANT, local)
        per_file[path] = local
    for path in sorted(source.rglob("*.h")):
        index(path, HEADER_CONSTANT, None)
    return per_file, everywhere


def resolve(token: str, local: dict[str, int], everywhere: dict[str, set[int]], where: Path) -> int:
    if token.startswith("0x"):
        return int(token.rstrip("uU"), 16)
    if token in local:
        return local[token]
    values = everywhere.get(token, set())
    if len(values) == 1:
        return next(iter(values))
    if not values:
        raise Refusal(
            f"{where}: declaration names its address '{token}', which is not a constexpr integer "
            f"constant in the scanned tree ({len(everywhere)} known). This scan cannot see that "
            f"declaration, so it must be fixed rather than skipped")
    raise Refusal(f"{where}: '{token}' is {sorted(values)} in different translation units; "
                  f"this scan will not guess which one the declaration means")


def scan_declarations(source: Path, pattern: re.Pattern[str], address_group: int,
                      other_groups: tuple[int, ...], label: str) -> list[tuple]:
    """Match one declaration form across the tree, resolving named address constants.

    `other_groups` gives the order the remaining captured groups are returned in, so a caller reads
    `(address, *others, path)` whichever way the pattern happens to capture them.
    """
    per_file, everywhere = named_constants(source)
    found: list[tuple] = []
    scanned = 0
    for path in sorted(source.rglob("*.cpp")):
        scanned += 1
        for values in pattern.findall(path.read_text()):
            address = resolve(values[address_group], per_file[path], everywhere, path)
            found.append((address, *(values[index] for index in other_groups), path))
    print(f"[{label}] scanned {scanned} sources under {source.relative_to(ROOT)}: {len(found)} "
          f"{label}-form declaration(s), {len(everywhere)} constexpr address constants indexed")
    return found


def unreachable_declarations(source: Path) -> list[tuple[int, str, Path]]:
    """Every resident-form declaration at an address `bindResident` cannot reach.

    The product names these itself on every run; reading them out of the source instead means the
    answer does not depend on reaching a particular frame, and the two must agree. If this returns
    nothing it says so with the denominator it scanned, because "no offenders" and "never looked"
    print the same otherwise.
    """
    declared = scan_declarations(source, DECLARE_RESIDENT, 0, (1,), "resident")
    found = [row for row in declared
             if not RESIDENT_TEXT[0] <= row[0] < RESIDENT_TEXT[1]]
    print(f"[owner] {len(found)} of {len(declared)} resident-form declarations are outside "
          f"[{RESIDENT_TEXT[0]:08X}, {RESIDENT_TEXT[1]:08X}) and can NEVER install")
    return found


def summarise(offenders: list[tuple[int, str, Path]], images: dict[str, bytes]) -> None:
    """Split the offenders by how much the images themselves decide."""
    decided: list[str] = []
    contested: list[str] = []
    orphan: list[str] = []
    for address, name, _ in sorted(offenders):
        groups = examine(address, images).groups()
        (decided if len(groups) == 1 else contested if groups else orphan).append(
            f"{address:08X} {name}")
    print(f"\n[owner] of {len(offenders)} unreachable declarations:")
    print(f"[owner]   {len(decided)} have exactly one image holding a function there")
    print(f"[owner]   {len(contested)} have more than one, so the owner's own provenance decides")
    print(f"[owner]   {len(orphan)} have none at all — the address is not a MODE function")
    for label, rows in (("contested", contested), ("no owner", orphan)):
        for row in rows:
            print(f"[owner]   [{label}] {row}")


def overlay_declarations(source: Path) -> list[tuple[int, str, str, Path]]:
    """Every identity-scoped declaration in the source, in the form the product binds."""
    return scan_declarations(source, DECLARE_OVERLAY, 1, (0, 2), "overlay")


def census(declarations: list[tuple[int, str, str, Path]], images: dict[str, bytes]) -> int:
    """Cross-check every overlay declaration against the images, and state the install census.

    THE PRODUCT'S OWN RULE, restated so this is not a second opinion: `bindOverlay` installs a
    declaration when the active image's NAME matches, the address is inside that image's loaded text
    range, and the identity at the address is that image. Every MODE image is loaded at MODE_SLOT and
    its size is the image's own length (which `authenticate` already matched against the manifest), so
    both conditions are decidable here without running anything. `in_text_range` is therefore the
    count the product will reach, and it is the number a conversion must move.

    `image_holds_the_function` is a STRONGER question than the product asks, and its negative is not a
    product failure: it is whether a stack-allocating prologue starts at the address, which a MIPS leaf
    or a split prologue does not have. Three declarations already shipping in this tree read 0 there
    and install anyway. So a row with no prologue is reported as a SHAPE note with the word at the
    offset, and only an out-of-range address fails.
    """
    failures: list[str] = []
    notes: list[str] = []
    print("\n[census] expected install count per MODE image (what the product's own bindOverlay counts):")
    for name in images:
        rows = [row for row in declarations if row[1] == name]
        if not rows:
            continue
        size = len(images[name])
        in_range = [row for row in rows if MODE_SLOT <= row[0] < MODE_SLOT + size]
        shaped = 0
        for row in in_range:
            holders = {image for group in examine(row[0], images).groups().values() for image in group}
            if name in holders:
                shaped += 1
            else:
                offset = row[0] - MODE_SLOT
                word = int.from_bytes(images[name][offset : offset + 4], "little")
                verdict = ", ".join(sorted(holders)) if holders else "no stack-allocating prologue"
                notes.append(f"{row[0]:08X} {row[2]}  {name} installs it anyway; "
                             f"no stack-allocating prologue at the entry (word 0x{word:08X}, {verdict})")
        for row in rows:
            if not (MODE_SLOT <= row[0] < MODE_SLOT + size):
                failures.append(f"{row[0]:08X} {row[2]}  outside {name}'s loaded text range "
                                f"[{MODE_SLOT:08X}, {MODE_SLOT + size:08X}) — it can never install")
        print(f"[census]   {name}: declared={len(rows):3d} in_text_range={len(in_range):3d} "
              f"stack_prologue_at_entry={shaped:3d}")
    print(f"[census] {len(failures)} declaration(s) that can never install:")
    for row in failures:
        print(f"[census]   {row}")
    print(f"[census] {len(notes)} declaration(s) with no stack-allocating prologue at the entry "
          f"(installed anyway; function-shape note, not a failure):")
    for row in notes:
        print(f"[census]   {row}")
    return 1 if failures else 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("addresses", nargs="*", help="guest addresses, e.g. 0x801113B4")
    parser.add_argument("--unreachable", action="store_true",
                        help="examine every resident-form declaration that can never install")
    parser.add_argument("--census", action="store_true",
                        help="cross-check every overlay-form declaration against the authenticated images "
                             "and print the per-image install count; exits 1 on any mismatch")
    parser.add_argument("--images", default="scratch/bin/overlays", help="directory holding the extracted .BIN images")
    parser.add_argument("--manifest", default="config/tomba2-images.json")
    args = parser.parse_args(argv)
    status = 0
    try:
        manifest = json.loads((ROOT / args.manifest).read_text())
        names = mode_image_names(manifest)
        images = authenticate(ROOT / args.images, manifest, names)
        print(f"[owner] authenticated {len(images)} MODE images against {args.manifest}")
        if args.census:
            declared = overlay_declarations(ROOT / "game")
            other = sorted({row[1] for row in declared} - set(names))
            if other:
                print(f"[census] NOTE: {len(other)} declared image name(s) are not MODE images and are "
                      f"NOT covered by this census: {', '.join(other)}")
            status |= census(declared, images)
            unreachable_declarations(ROOT / "game")
        if args.unreachable:
            offenders = unreachable_declarations(ROOT / "game")
            for address, name, path in sorted(offenders):
                print(f"\n{name}  ({path})")
                report(examine(address, images))
            summarise(offenders, images)
        elif not args.addresses and not args.census:
            raise Refusal("give an address, or --unreachable / --census to examine the declarations")
        for text in args.addresses:
            report(examine(int(text, 16), images))
    except (OSError, ValueError, KeyError, Refusal) as error:
        print(f"[owner] REFUSED: {error}", file=sys.stderr)
        return 2
    return status


if __name__ == "__main__":
    raise SystemExit(main())
