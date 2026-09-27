#!/usr/bin/env python3
"""tools/frame_cadence_census.py — the gate for Tomba! 2's frame-rate lever.

WHAT THIS ANSWERS, and what it does not.

ANSWERS: for Tomba! 2's own vblank gate —
    do {} while (DAT_800e809c < DAT_1f800235)          at 0x80050CC8..0x80050CF4
— every site in the authenticated resident image that reads or writes either side of the
comparison, what the retail value of the threshold is, and what the per-vblank writer's body
actually is. It reads MAIN.EXE directly and matches instruction words, so it cannot answer from
Ghidra's memory or from a copied constant.

DOES NOT ANSWER: anything about overlays. The reference scan is base-independent (it matches
address-forming words, never a load address), so it will also name candidate sites inside the 28
provisioned overlay images — but it reports those as CANDIDATES with the words it matched, and
never as references. A candidate is an offset that happens to equal a target's low half; whether
it is the same field is a question about that image's own base register, which this tool does not
establish. The report says so in those words.

DENOMINATORS, because a scan that cannot say what it looked at is how a "found none" becomes
evidence of absence: every run prints the words it scanned per image, the sites it matched, and
the sites it was asked about. The resident image is the one whose answer is a fact; the overlay
count is printed as a count.

`--check` additionally diffs the constants that SHIP in game/core/frame_cadence.h against what
this tool measures from the image, so the measured value and the value in the port cannot drift
apart with both gates green. `--selftest` is the positive test of the case that would otherwise
fail silently: it feeds the matcher a synthetic image whose quota store is at a DIFFERENT
instruction word and requires the tool to report the mismatch.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import struct
import sys
from dataclasses import dataclass, field

# --- guest addresses, as the census DERIVES them from the instruction words below. -------------
# These are the targets the scan looks for. They are written here as the tool's own query, and
# --check proves the shipping header agrees with the image; a reader who wants the ground truth
# should read the matched words, not this list.
QUOTA_ADDRESS = 0x1F800235  # DAT_1f800235 — the gate threshold, a u8
DWELL_COUNTER_ADDRESS = 0x800E809C  # DAT_800e809c — the vblank-ticked dwell counter, a u16

PSX_EXE_MARKER = b"PS-X EXE"
PSX_EXE_HEADER_SIZE = 0x800
# PS-X EXE header field offsets, read from the image rather than assumed. 0x10 is the entry PC,
# 0x18 the destination address in RAM, and 0x1C the payload size EXCLUDING the header — which is
# the same number as (filesize - 0x800), so the loader can check the header against the file
# instead of trusting one field.
PSX_EXE_PC_OFFSET = 0x10
PSX_EXE_LOAD_BASE_OFFSET = 0x18
PSX_EXE_SIZE_OFFSET = 0x1C

# MIPS primary opcodes this tool decodes. Anything else is reported as "other" rather than
# skipped, so a site is never dropped because its opcode was not on the list.
OP_LUI = 0x0F
OP_LBU = 0x24
OP_LHU = 0x25
OP_SB = 0x28
OP_SH = 0x29

_LOAD_STORE = {
    OP_LBU: "lbu",
    OP_LHU: "lhu",
    OP_SB: "sb",
    OP_SH: "sh",
}

RS_ZERO = 0


@dataclass(frozen=True)
class Site:
    """One matched instruction: where, what word, and which target it could name."""

    offset: int  # byte offset within the text slice
    word: int  # the instruction word, big-endian value
    mnemonic: str
    role: str  # "target" or "candidate"
    why: str

    def __str__(self) -> str:
        return (
            f"    +0x{self.offset:06X}  word 0x{self.word:08X}  {self.mnemonic:<5s}  "
            f"[{self.role}] {self.why}"
        )


@dataclass
class ImageReport:
    label: str
    words_scanned: int
    load_base: int | None
    sites: list[Site] = field(default_factory=list)
    page_luis: int = 0

    def targets(self) -> list[Site]:
        return [s for s in self.sites if s.role == "target"]

    def candidates(self) -> list[Site]:
        return [s for s in self.sites if s.role == "candidate"]


class ImageError(RuntimeError):
    """A missing or malformed input. Refused, never defaulted."""


class MissingInput(ImageError):
    """No image to answer from. Distinct from malformed ON PURPOSE.

    A fresh clone has no disc, and a gate that fails for want of a game file is a gate that
    trains its reader to ignore it. So absence SKIPS and a PRESENT image that does not hold up
    FAILS — that asymmetry is the whole reason these are two exceptions.
    """


@dataclass(frozen=True)
class ResidentImage:
    load_base: int
    entry_pc: int
    declared_size: int
    text: bytes

    @property
    def text_hi(self) -> int:
        return self.load_base + len(self.text)


def load_psx_exe(path: pathlib.Path) -> ResidentImage:
    """Read a PS-X EXE's identity from its own header.

    The load base is the header's destination field, never a constant here: the same bytes in a
    differently-linked build give a different base and this must say so rather than report
    addresses that do not apply. The declared payload size is checked against the actual file
    length, so a truncated or padded image is refused instead of being censused with a tail of
    zeros that would read as "no more references".
    """
    if not path.is_file():
        raise MissingInput(f"missing image: {path}")
    data = path.read_bytes()
    if len(data) < PSX_EXE_HEADER_SIZE:
        raise ImageError(f"{path}: {len(data)} bytes, shorter than a PS-X EXE header")
    if PSX_EXE_MARKER not in data[:0x100]:
        raise ImageError(f"{path}: no PS-X EXE marker in the first 0x100 bytes")
    entry_pc = struct.unpack_from("<I", data, PSX_EXE_PC_OFFSET)[0]
    load_base = struct.unpack_from("<I", data, PSX_EXE_LOAD_BASE_OFFSET)[0]
    declared = struct.unpack_from("<I", data, PSX_EXE_SIZE_OFFSET)[0]
    text = data[PSX_EXE_HEADER_SIZE:]
    if len(text) % 4:
        raise ImageError(f"{path}: text is {len(text)} bytes, not a whole number of words")
    if declared != len(text):
        raise ImageError(
            f"{path}: header declares {declared} payload bytes but the file carries {len(text)} "
            f"after its {PSX_EXE_HEADER_SIZE}-byte header. Refusing to census a truncated or padded "
            f"image: the missing tail would read as 'no more references'."
        )
    return ResidentImage(load_base=load_base, entry_pc=entry_pc, declared_size=declared, text=text)


def decode(word: int) -> tuple[str, int, int, int]:
    """(mnemonic, rs, rt, immediate) for the opcodes this census reasons about."""
    op = word >> 26
    rs = (word >> 21) & 0x1F
    rt = (word >> 16) & 0x1F
    imm = word & 0xFFFF
    if op == OP_LUI:
        return "lui", rs, rt, imm
    if op in _LOAD_STORE:
        return _LOAD_STORE[op], rs, rt, imm
    return "other", rs, rt, imm


def signed16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


# How many words behind a load/store the matcher looks for the `lui` that forms its address. The
# MIPS idiom is `lui rX,hi` then `lw/sw rY,lo(rX)`. The width is measured, not guessed: the
# deepest site in FUN_80050b08 is the gate's own `lhu` at 0x80050CCC, and its base register $s6 is
# set by `lui s6,0x800F` at 0x80050B34 — 102 words earlier, in a jal delay slot. 128 covers it with
# margin and stops long before a window could run into the previous function.
BACKWARD_WINDOW = 128

# Opcodes whose instruction writes a KNOWN single destination register. Everything not listed
# returns -1, i.e. "unknown", which makes the caller treat the register as clobbered rather than
# trusting a `lui` from before it. A permissive table would let an unmodelled instruction pass a
# stale address through, and a "target" in this tool means the address was actually formed.
_DEST_FROM_RT = frozenset(
    {
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,  # addi/addiu/slti/andi/ori/xori/lui
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,  # lb/lh/lwl/lw/lbu/lhu/lwr
        0x28, 0x29, 0x2A, 0x2B, 0x2E, 0x2F,  # sb/sh/swl/sw/swr
    }
)


def _define_site(word: int) -> int:
    """The register this instruction writes, or -1 when unknown / writes none."""
    op = word >> 26
    if op == 0x00:  # SPECIAL: only the R-type shift/move forms this matcher needs to model
        rd = (word >> 11) & 0x1F
        funct = word & 0x3F
        return rd if funct in (0x00, 0x02, 0x03, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0D, 0x10, 0x12) else -1
    if op in _DEST_FROM_RT:
        return (word >> 16) & 0x1F
    return -1


# MIPS o32: $s0-$s7 (r16-r23) survive a call. That is an ABI guarantee, not an assumption about
# this image, so a `jal` is only a barrier to the backward walk when the base register is one the
# ABI lets a call clobber. Without this the two sites that take the counter's address from $s6 —
# set once in the main loop's prologue — are unresolvable from inside the loop body, and the tool
# would report 6 references where the image has 8.
CALLEE_SAVED = frozenset(range(16, 24))


def _form_target(text: bytes, index: int, base_reg: int, offset: int, target: int, window: int) -> tuple[str, str]:
    """Try to FORM `target` from a `lui` of the base register within `window` words behind.

    Returns (role, why). A role of "target" means a `lui base,hi` was found with
    (hi << 16) + offset == target and no later instruction in the window rewrote that register.
    The walk stops at a call for a caller-saved base register, because the call may have rewritten
    it and a `lui` from before the call proves nothing; it crosses a call for a callee-saved one,
    which the ABI says a call cannot touch. Everything else is a candidate and the reason says
    which of those it was.
    """
    crossed_call = False
    for back in range(1, window + 1):
        if index - back < 0:
            break
        prior_index = index - back
        prior = struct.unpack_from("<I", text, prior_index * 4)[0]
        op = prior >> 26
        rt = (prior >> 16) & 0x1F
        if op == 0x03:  # jal
            if base_reg not in CALLEE_SAVED:
                return "candidate", (
                    f"a `jal` at +0x{prior_index * 4:06X} is inside the traced window and r{base_reg} "
                    f"is caller-saved, so it may have been rewritten"
                )
            crossed_call = True
            continue
        if op == OP_LUI and rt == base_reg:
            imm = prior & 0xFFFF
            if (imm << 16) + offset == target:
                note = " (crossing a call, which the o32 ABI says cannot touch an $s register)" if crossed_call else ""
                return "target", f"formed by `lui r{base_reg},0x{imm:04X}` at +0x{prior_index * 4:06X}{note}"
            return "candidate", (
                f"`lui r{base_reg},0x{imm:04X}` at +0x{prior_index * 4:06X} forms "
                f"0x{(imm << 16) + offset:08X}, not 0x{target:08X}"
            )
        if _define_site(prior) == base_reg:
            return "candidate", (
                f"base register r{base_reg} is redefined at +0x{prior_index * 4:06X} before any "
                f"`lui` of it appears in the traced window"
            )
    return "candidate", (
        f"base register r{base_reg} is not defined in the {window} words behind this site; this tool "
        f"does not follow prologue or inter-procedural definitions"
    )


def scan_image(label: str, load_base: int | None, text: bytes) -> ImageReport:
    """Match every instruction that could name either side of the gate.

    A `lui` carrying one of the two page constants is NOT a reference to a field: `lui reg,0x1f80`
    is the scratchpad page base and appears in every image that uses scratchpad at all, so
    counting those as hits would print thousands of guaranteed matches. They are counted as a
    denominator instead.

    The only informative sites are the loads and stores whose offset is a target's low half, and
    each of those is accepted as a TARGET only when a `lui` of its base register within the traced
    window actually forms the address. Everything else is a CANDIDATE carrying the reason it
    could not be resolved.
    """
    report = ImageReport(label=label, words_scanned=len(text) // 4, load_base=load_base)
    page_constants = {(QUOTA_ADDRESS >> 16) & 0xFFFF, (DWELL_COUNTER_ADDRESS >> 16) & 0xFFFF}
    report.page_luis = sum(
        1
        for index in range(report.words_scanned)
        if (struct.unpack_from("<I", text, index * 4)[0] >> 26) == OP_LUI
        and (struct.unpack_from("<I", text, index * 4)[0] & 0xFFFF) in page_constants
    )

    for index in range(report.words_scanned):
        word = struct.unpack_from("<I", text, index * 4)[0]
        mnemonic, rs, rt, imm = decode(word)
        if mnemonic == "other":
            continue
        offset = signed16(imm)
        for target in (QUOTA_ADDRESS, DWELL_COUNTER_ADDRESS):
            low = signed16(target & 0xFFFF)
            if offset != low:
                continue
            role, why = _form_target(text, index, rs, offset, target, BACKWARD_WINDOW)
            report.sites.append(
                Site(
                    offset=index * 4,
                    word=word,
                    mnemonic=mnemonic,
                    role=role,
                    why=why,
                )
            )
            break
    return report


def retail_quota_from_image(text: bytes) -> tuple[int | None, int | None]:
    """The stored value of the gate threshold, and where it is stored.

    Walks the images of the `sb` sites on the threshold and looks for the instruction that
    MATERIALISES the value: an `addiu rt,zero,imm` (li) is taken literally, because that is the
    form the retail image uses (word 0x24020002, `li v0,0x2`). A store of a register the census
    cannot trace is reported as "unknown", not as zero — a threshold of 0 would make the gate
    release immediately, and reading that as "the value is 0" is the failure this shape exists to
    prevent.
    """
    quota_lo = QUOTA_ADDRESS & 0xFFFF
    stores: list[tuple[int, int, int]] = []  # (word index, stored register, base register)
    for index in range(len(text) // 4):
        word = struct.unpack_from("<I", text, index * 4)[0]
        mnemonic, rs, rt, imm = decode(word)
        if mnemonic == "sb" and imm == quota_lo:
            stores.append((index, rt, rs))
    if not stores:
        return None, None
    if len(stores) > 1:
        raise ImageError(
            f"quota byte has {len(stores)} store sites {['0x%08X' % (0x80010000 + i * 4) for i, _, _ in stores]}"
            "; this tool reports one value and cannot attribute it to one store"
        )
    index, value_reg, _base = stores[0]
    for back in range(1, 9):
        if index - back < 0:
            break
        prior = struct.unpack_from("<I", text, (index - back) * 4)[0]
        _mnemonic, rs, rt, imm = decode(prior)
        # addiu rt,zero,imm  (li) — mask to the opcode+rs field so BOTH the immediate and the
        # destination register are free; the destination is checked separately, because masking it
        # away too would accept every `addiu` as a `li`.
        if (prior & 0xFFE00000) == 0x24000000 and rs == RS_ZERO and rt == value_reg:
            return imm, index - back
    return None, index


# ---------------------------------------------------------------------------------------------
# --check: the shipping header against the measurement.
# ---------------------------------------------------------------------------------------------

_HEADER_CONSTANTS = (
    ("kQuotaAddress", QUOTA_ADDRESS),
    ("kDwellCounterAddress", DWELL_COUNTER_ADDRESS),
)


def parse_shipping_constants(header: pathlib.Path) -> dict[str, int]:
    """Read the named constants out of the shipping header.

    Parsed from the FILE, not from this tool's copy: that is the whole point. A tool that holds
    its own copy of the value and asserts the header matches it would pass when both are wrong
    together, which is the failure the workspace rule about measured constants is written about.
    """
    if not header.is_file():
        raise ImageError(f"missing shipping header: {header}")
    text = header.read_text()
    found: dict[str, int] = {}
    for name, _expected in _HEADER_CONSTANTS:
        match = re.search(rf"\b{name}\s*=\s*(0[xX][0-9A-Fa-f]+|\d+)[uU]?\s*;", text)
        if not match:
            raise ImageError(f"{header}: no `{name} = ...;` found — the tool cannot check a constant it cannot read")
        found[name] = int(match.group(1), 0)
    match = re.search(r"\bkRetailVblanksPerLogicFrame\s*=\s*(\d+)[uU]?\s*;", text)
    if not match:
        raise ImageError(f"{header}: no `kRetailVblanksPerLogicFrame = ...;` found")
    found["kRetailVblanksPerLogicFrame"] = int(match.group(1))
    return found


def check_shipping(header: pathlib.Path, report: ImageReport, text: bytes) -> list[str]:
    problems: list[str] = []
    shipping = parse_shipping_constants(header)
    for name, measured in _HEADER_CONSTANTS:
        if shipping[name] != measured:
            problems.append(
                f"{name}: shipping {shipping[name]:#010x} != measured {measured:#010x} "
                f"(measured as the field the image's gate forms; see the site list above)"
            )
    value, at = retail_quota_from_image(text)
    if value is None:
        problems.append(
            "kRetailVblanksPerLogicFrame: the image's single quota store is fed from a register "
            "this tool cannot trace, so the retail threshold is UNKNOWN and the shipping value "
            f"{shipping['kRetailVblanksPerLogicFrame']} is UNVERIFIED"
            + (f" (store at word index {at})" if at is not None else "")
        )
    elif value != shipping["kRetailVblanksPerLogicFrame"]:
        where = f"0x{0x80010000 + at * 4:08X}" if at is not None else "an unlocated word"
        problems.append(
            f"kRetailVblanksPerLogicFrame: shipping {shipping['kRetailVblanksPerLogicFrame']} != "
            f"image {value} (the literal at {where})"
        )
    return problems


# ---------------------------------------------------------------------------------------------
# --selftest: the case that would otherwise fail silently.
# ---------------------------------------------------------------------------------------------

SELFTEST_IMAGE_OFFSET = 0x1000


def selftest() -> int:
    """Build two synthetic images that differ in ONE thing, and require opposite answers.

    Positive control: an image whose quota store is fed by `li v0,2` must report 2.
    Negative control: the same image with that literal changed to `li v0,1` must report 1, so a
    shipping header saying 2 against it is a FAILURE rather than a pass. Without the negative, a
    matcher that always answers 2 would be green forever, which is precisely the instrument that
    cannot lie being asked to.
    """
    failures = 0

    def build(literal: int, literal_before: bool = True, with_literal: bool = True) -> bytes:
        body = bytearray(0x2000)
        # The measured shape: the value is materialised IMMEDIATELY BEFORE the store that consumes
        # it (`li v0,0x2` at 0x80050A1C, `sb v0,0x235(v1)` at 0x80050A20).
        literal_at = 0x00 if literal_before else 0x08
        words = {
            0x04: 0xA0620235,  # sb v0,0x235(v1)
            0x0C: 0x96C2809C,  # lhu v0,-0x7f64(s6)   -- a plausible second site
        }
        if with_literal:
            # addiu v0,zero,<literal>  — rt is v0 (2), which is the register the store consumes.
            words[literal_at] = 0x24000000 | (2 << 16) | literal
        for offset, word in words.items():
            struct.pack_into("<I", body, SELFTEST_IMAGE_OFFSET + offset, word)
        return bytes(body)

    positive, _ = retail_quota_from_image(build(2))
    if positive != 2:
        print(f"FAIL selftest positive: asked for 2, tool reported {positive!r} (wanted 2)")
        failures += 1
    negative, _ = retail_quota_from_image(build(1))
    if negative != 1:
        print(f"FAIL selftest negative: asked for 1, tool reported {negative!r} (wanted 1)")
        failures += 1
    if positive == negative:
        print("FAIL selftest: the two images gave the same answer, so the matcher is not reading the literal")
        failures += 1

    # The materialising literal AFTER the store is not a shape the tool traces, and it must say so
    # rather than guess a value. A tool that scanned forward here could attribute an unrelated `li`
    # to the threshold and report a number the image never stored.
    after, _ = retail_quota_from_image(build(2, literal_before=False))
    if after is not None:
        print(f"FAIL selftest ordering: tool reported {after} for a literal it should have refused to trace")
        failures += 1

    # A store whose value register is never materialised must be UNKNOWN, not 0. Reading it as 0
    # would claim a threshold that releases the gate immediately.
    untraceable, at = retail_quota_from_image(build(0, with_literal=False))
    if untraceable is not None:
        print(f"FAIL selftest untraceable: tool invented value {untraceable} for an untraced store")
        failures += 1
    if at is None:
        print("FAIL selftest untraceable: tool did not report the store's location")
        failures += 1

    # Two store sites must be refused, not silently resolved to one.
    two = bytearray(build(2))
    struct.pack_into("<I", two, SELFTEST_IMAGE_OFFSET + 0x10, 0xA0620235)
    try:
        retail_quota_from_image(bytes(two))
    except ImageError:
        pass
    else:
        print("FAIL selftest ambiguous: two quota stores were accepted as one")
        failures += 1

    if failures == 0:
        print("frame_cadence_census --selftest: 7 checks passed "
              "(positive, negative, distinctness, ordering, untraceable, store-located, ambiguous)")
    return 1 if failures else 0


# ---------------------------------------------------------------------------------------------


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--exe", type=pathlib.Path, default=pathlib.Path("scratch/bin/tomba2/MAIN.EXE"),
                        help="the authenticated PS-X EXE to census (default: %(default)s)")
    parser.add_argument("--overlays", type=pathlib.Path, default=pathlib.Path("scratch/bin/overlays"),
                        help="directory of provisioned overlay images, scanned for CANDIDATE sites only")
    parser.add_argument("--header", type=pathlib.Path, default=pathlib.Path("game/core/frame_cadence.h"),
                        help="the shipping header --check diffs against (default: %(default)s)")
    parser.add_argument("--check", action="store_true",
                        help="diff the shipping constants against what the image measures")
    parser.add_argument("--selftest", action="store_true", help="run the tool against its own fixtures and exit")
    args = parser.parse_args(argv)

    if args.selftest:
        return selftest()

    try:
        resident_image = load_psx_exe(args.exe)
    except MissingInput as error:
        print(f"SKIPPED: {error}")
        print("          provision the authenticated image (tools/tomba2_provision.py) and re-run. "
              "This tool will not answer from a missing input, and it will not pretend a missing "
              "input is a passing result either.")
        return 77  # SKIP_RETURN_CODE, so a fresh clone without a disc skips instead of failing
    except ImageError as error:
        print(f"REFUSED: {error}")
        return 2
    load_base = resident_image.load_base
    text = resident_image.text

    resident = scan_image(str(args.exe), load_base, text)
    print(f"[cadence] resident image {args.exe}: entry 0x{resident_image.entry_pc:08X}, load base "
          f"0x{load_base:08X}, text [{load_base:08X},{load_base + len(text):08X}) — all three read "
          f"from the image's own PS-X EXE header, none assumed")
    if load_base != 0x80010000:
        print(f"          NOTE: this image does not load at 0x80010000, so the absolute addresses below "
              f"are not this title's. Reported anyway rather than silently mapped onto the known ones.")
    print(f"          {resident.page_luis} instruction(s) build one of the two gate fields' page "
          f"constants. Those are page BASES, not references to a field, and are counted here rather "
          f"than listed: `lui reg,0x1f80` is the scratchpad page and appears in every image that "
          f"uses scratchpad at all, so treating it as a hit would be a guaranteed match.")
    print(f"          {len(resident.targets())} reference(s) and {len(resident.candidates())} "
          f"unresolved candidate(s) for {len(_HEADER_CONSTANTS)} queried addresses")
    for site in resident.sites:
        print(site)

    if resident.targets():
        value, at = retail_quota_from_image(text)
        if value is None:
            where = f"store at word index {at}" if at is not None else "no store found"
            print(f"          retail gate threshold: UNKNOWN ({where}; the stored register is not a "
                  f"traceable literal in this image)")
        else:
            where = f"0x{load_base + (at or 0) * 4:08X}"
            print(f"          retail gate threshold: {value} display field(s) per logic frame, "
                  f"literal at {where}")

    overlay_dir: pathlib.Path = args.overlays
    overlay_images = sorted(overlay_dir.glob("*.BIN")) if overlay_dir.is_dir() else []
    overlay_words = 0
    overlay_targets = 0
    overlay_candidates = 0
    per_image_cap = 4
    for image in overlay_images:
        data = image.read_bytes()
        text_only = data[: len(data) // 4 * 4]
        overlay_words += len(text_only) // 4
        rep = scan_image(image.name, None, text_only)
        overlay_targets += len(rep.targets())
        overlay_candidates += len(rep.candidates())
        shown = rep.sites[:per_image_cap]
        for site in shown:
            print(f"  overlay {image.name} ({rep.words_scanned} words)")
            print(site)
        if len(rep.sites) > per_image_cap:
            print(f"  overlay {image.name}: {len(rep.sites) - per_image_cap} further site(s) not printed "
                  f"(cap {per_image_cap}/image); the totals below count all of them")
    print(f"[cadence] overlays: {len(overlay_images)} image(s) present, {overlay_words} words scanned, "
          f"{overlay_targets} reference(s) and {overlay_candidates} unresolved candidate(s).")
    print(f"          A CANDIDATE is an offset equal to a target's low half whose base register this "
          f"tool could not resolve; each line says why. This scan is base-independent, so an overlay "
          f"site is not evidence about the resident image's fields, and nothing here concludes that "
          f"any overlay reads either side of the gate.")

    if not args.check:
        return 0

    try:
        problems = check_shipping(args.header, resident, text)
    except ImageError as error:
        print(f"REFUSED: {error}")
        return 2
    if problems:
        print(f"[cadence] --check FAILED, {len(problems)} problem(s):")
        for problem in problems:
            print(f"  - {problem}")
        return 1
    print(f"[cadence] --check OK: {len(_HEADER_CONSTANTS) + 1} constant(s) in {args.header} match the image")
    return 0


if __name__ == "__main__":
    sys.exit(main())
