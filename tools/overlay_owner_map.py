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

DECLARE_RESIDENT = re.compile(r"declareOverride\(\s*(0x[0-9A-Fa-f]+)u?\s*,\s*\"([^\"]*)\"")

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


def unreachable_declarations(source: Path) -> list[tuple[int, str, Path]]:
    """Every resident-form declaration at an address `bindResident` cannot reach.

    The product names these itself on every run; reading them out of the source instead means the
    answer does not depend on reaching a particular frame, and the two must agree. If this returns
    nothing it says so with the denominator it scanned, because "no offenders" and "never looked"
    print the same otherwise.
    """
    found: list[tuple[int, str, Path]] = []
    scanned = 0
    for path in sorted(source.rglob("*.cpp")):
        scanned += 1
        for address_text, name in DECLARE_RESIDENT.findall(path.read_text()):
            address = int(address_text, 16)
            if not RESIDENT_TEXT[0] <= address < RESIDENT_TEXT[1]:
                found.append((address, name, path.relative_to(ROOT)))
    print(f"[owner] scanned {scanned} sources under {source.relative_to(ROOT)}: "
          f"{len(found)} resident-form declaration(s) outside [{RESIDENT_TEXT[0]:08X}, {RESIDENT_TEXT[1]:08X})")
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


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("addresses", nargs="*", help="guest addresses, e.g. 0x801113B4")
    parser.add_argument("--unreachable", action="store_true",
                        help="examine every resident-form declaration that can never install")
    parser.add_argument("--images", default="scratch/bin/overlays", help="directory holding the extracted .BIN images")
    parser.add_argument("--manifest", default="config/tomba2-images.json")
    args = parser.parse_args(argv)
    try:
        manifest = json.loads((ROOT / args.manifest).read_text())
        names = mode_image_names(manifest)
        images = authenticate(ROOT / args.images, manifest, names)
        print(f"[owner] authenticated {len(images)} MODE images against {args.manifest}")
        if args.unreachable:
            offenders = unreachable_declarations(ROOT / "game")
            for address, name, path in sorted(offenders):
                print(f"\n{name}  ({path})")
                report(examine(address, images))
            summarise(offenders, images)
        elif not args.addresses:
            raise Refusal("give an address, or --unreachable to examine every offending declaration")
        for text in args.addresses:
            report(examine(int(text, 16), images))
    except (OSError, ValueError, KeyError, Refusal) as error:
        print(f"[owner] REFUSED: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
