#!/usr/bin/env python3
"""Census the Tomba! 2 horizontal-visibility culling owners, in the authenticated images.

WHY THIS EXISTS. `docs/codemap.md` gives Tomba! 2 widescreen the job of "owning culling", and the
workspace discriminator `WIDENED BUT ... MARGIN IS NOT COVERAGE` exists because a title can widen the
frustum correctly and still leave the new margins empty. The engine's screen-edge bounds are the
narrowest thing that decides what may appear in a margin, so they have to be COUNTED, not guessed.

THIS IS A CENSUS, NOT A LIST. Every run prints what it scanned and what it matched, per image and in
total, and refuses an image set that is not the manifest's. A tool that printed the two hits its
author already knew about would be indistinguishable from a tool that looked everywhere.

COVERAGE IS MEASURED, NOT ASSUMED. This title ships code in FOUR image families at THREE different
RAM bases -- the resident MAIN.EXE, the 23 MODE overlays SOP/A00..A0L, the two AREA-slot overlays
OPN/CRD, and the three stage overlays START/DEMO/GAME -- so a resident-only scan is incomplete by
construction, and the tool names every family with its byte count. Within an image the scan is
CONTROL-FLOW REACHABILITY from `addiu sp,sp,-N` prologues rather than a blind linear sweep, so data
words embedded in a code image (jump tables, filename strings, node tables) are counted as unreached
instead of being disassembled into false hits. Both numbers are printed, and they sum to the image.

THE FOUR SCANS.
  A. immediates that are 4:3-frame-derived: the right edge (320), the bottom edge (240), the
     projection centre X/Y (160/120) and each one's 16.16 form, in any instruction that carries an
     immediate. This is the denominator for "compared against a screen edge". 160 and 120 are in the
     set because the projection centre IS the half-extent, and a half-extent is a quantity that has
     to scale with the canvas -- a literal half-width is the defect this census is hunting.
  B. loads out of the draw environment the guest itself published. `PutDrawEnv` (RE'd in
     game/render/wide_re_gpu_putdrawenv.cpp) copies the 92-byte DRAWENV to the "current env" cache at
     0x800A59B0, and its +4/+6 are the clip width/height. An owner that reads its bound from there
     DERIVES it, and a derived bound already follows the widened window; an owner that compares
     against a literal cannot. Separating those two classes is the deliverable.
  C. the in-image call graph, so a candidate can be named by its callers. Without it a hit is an
     address, not a claim.
  D. writes to the GTE screen-centre / projection-plane control registers, so the census can say
     which functions PUBLISH the projection and therefore own the widening.

USAGE
    uv run --frozen python tests/horizontal_cull_census.py --scan a
    uv run --frozen python tests/horizontal_cull_census.py --scan b
    uv run --frozen python tests/horizontal_cull_census.py --refs 0x8003B320
    uv run --frozen python tests/horizontal_cull_census.py --selfcheck
    uv run --frozen python tests/horizontal_cull_census.py --coverage-only

THE SELF-CHECK IS THE POINT. A census that has only ever printed hits is not evidence. `--selfcheck`
builds synthetic images here and runs the matcher over them with both polarities: a constant that MUST
match, and near neighbours (a register-register compare, a non-frame literal, a mask that happens to
spell a frame constant) that must NOT. The first version of this scanner matched on mnemonic text,
which read capstone's `lui` operand as a shifted value and so saw no 16.16 constant at all; a
hand-typed positive control then asserted that blindness. Both are fixed, and the control now builds
its encodings from (opcode, registers, immediate) so it cannot assert a wrong word.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from collections import defaultdict

from capstone import CS_ARCH_MIPS, CS_MODE_LITTLE_ENDIAN, CS_MODE_MIPS32, Cs

# The scanner owns the binary; this file owns the command line, the reports and the self-check. It
# imports the names it needs explicitly rather than reaching through the module, so a rename in the
# scanner is a NameError here at import time instead of a silent behaviour change in a report.
from hcull_scanner import (
    DRAW_ENV_CACHE,
    DRAW_ENV_LEN,
    FRAME_CONSTANTS,
    FUNCT_SLT,
    FUNCT_SLTU,
    OP_ADDIU,
    OP_ANDI,
    OP_LHU,
    OP_LUI,
    OP_LW,
    OP_ORI,
    OP_SLTI,
    OP_SLTIU,
    OP_SPECIAL,
    OVERLAYS,
    Function,
    Image,
    Index,
    Refusal,
    build_indexes,
    load_images,
    report_bodies,
    scan_callers,
    scan_draw_env_loads,
    scan_frame_constants,
    scan_projection_writers,
    verify_resident_mapping,
)
from hcull_scanner import _lui_pair  # the self-check asserts the lui+ori pair resolves


# -- reporting -----------------------------------------------------------------------------------
def report_coverage(indexes: list[Index]) -> None:
    print("== coverage: images, bytes, words attributed to a walked function body")
    totals = [0, 0, 0, 0]
    per_family: dict[str, list[int]] = defaultdict(lambda: [0, 0, 0, 0])
    for index in indexes:
        reached = len(index.attributed)
        escaped = len(index.escaped)
        totals[0] += len(index.image.data)
        totals[1] += index.image.word_count
        totals[2] += reached
        totals[3] += escaped
        fam = per_family[index.image.family]
        fam[0] += len(index.image.data)
        fam[1] += index.image.word_count
        fam[2] += reached
        fam[3] += escaped
        print(
            f"  {index.image.name:9s} {index.image.family:8s} base {index.image.base:08X} "
            f"{len(index.image.data):7d} B {index.image.word_count:6d} words "
            f"{len(index.prologues):5d} prolog {len(index.functions):5d} fns "
            f"{reached:6d} reached ({100.0 * reached / index.image.word_count:5.1f}%) "
            f"{index.image.word_count - reached:6d} unreached {escaped:3d} walk(es) hit the cap"
        )
    print("  by family:")
    for family, (nbytes, nwords, reached, escaped) in sorted(per_family.items()):
        print(
            f"    {family:8s} {nbytes:8d} B {nwords:7d} words {reached:7d} reached "
            f"({100.0 * reached / nwords:5.1f}%) {nwords - reached:7d} unreached {escaped:3d} capped"
        )
    print(
        f"  TOTAL {len(indexes)} images {totals[0]} B = {totals[1]} words; "
        f"{totals[2]} attributed to a walked body ({100.0 * totals[2] / totals[1]:.1f}%); "
        f"{totals[1] - totals[2]} unreached (data/jump tables/strings); {totals[3]} walks hit the cap"
    )


def report_hits(indexes: list[Index], hits: list[dict], label: str, context: int) -> None:
    by_image: dict[str, int] = defaultdict(int)
    by_value: dict[int, int] = defaultdict(int)
    for hit in hits:
        by_image[hit["image"].name] += 1
        by_value[hit["value"]] += 1
    report_coverage(indexes)
    print(f"== {label}: {len(hits)} site(s)")
    unresolved = getattr(scan_draw_env_loads, "unresolved", None)
    if unresolved is not None:
        print(
            f"   NOT COUNTED: {unresolved} load(s) with a base register this scan cannot resolve. "
            f"A register-relative load's effective address is base+imm and the base is not a "
            f"constant here, so whether it names the draw environment is unanswerable from this scan; "
            f"they are not silently dropped and not counted as hits either."
        )
    for value, count in sorted(by_value.items()):
        print(f"  constant {value} (0x{value:X}): {count} site(s)")
    for name, count in sorted(by_image.items()):
        print(f"  {name:9s}: {count} site(s)")
    for hit in hits:
        index = next(i for i in indexes if i.image is hit["image"])
        print(
            f"  {hit['image'].name:9s} {hit['addr']:08X} (body {hit['fn']:08X}) "
            f"{hit['value']} via {hit['how']}: {hit['text']}"
        )
        if context <= 0:
            continue
        start = (hit["addr"] - index.image.base) // 4
        for off in range(start - context, start + 3):
            if not 0 <= off < len(index.words):
                continue
            marker = ">>" if off == start else "  "
            print(f"    {marker} {index.image.base + 4 * off:08X}  {index.text_at(off)}")
# -- self-check ----------------------------------------------------------------------------------
PROLOGUE = 0x27BDFFE0  # addiu $sp, $sp, -0x20
EPILOGUE = 0x27BD0020  # addiu $sp, $sp, 0x20
JR_RA = 0x03E00008


def _encode(opcode: int, rs: int, rt: int, imm: int) -> int:
    return ((opcode & 0x3F) << 26) | ((rs & 0x1F) << 21) | ((rt & 0x1F) << 16) | (imm & 0xFFFF)


def _special(funct: int, rs: int, rt: int, rd: int = 0) -> int:
    return (OP_SPECIAL << 26) | (rs << 21) | (rt << 16) | (rd << 11) | funct


def _index(body: list[int], label: str, md: Cs, base: int = 0x80010000) -> Index:
    data = b"".join(w.to_bytes(4, "little") for w in body)
    index = Index(Image(label, "synthetic", base, data), md)
    index.walk()
    return index


def selfcheck() -> int:
    """Positive AND negative controls for the matcher, on images built here.

    Every encoding is BUILT from (opcode, registers, immediate) rather than typed as hex. The first
    version of this control hand-typed its words and two were wrong -- a `sltiu` whose immediate field
    was 0, and a `lui` carrying 0x1400 -- and the control was asserting the matcher's blindness as if
    it were the matcher's contract. A control that can encode the wrong instruction is not a control.
    """
    md = _decoder()
    cases: list[tuple[str, object, object, str]] = []

    def scan(body: list[int], label: str = "SYNTH") -> list[dict]:
        return scan_frame_constants([_index(body, label, md)])

    # POSITIVE: the right edge, as a zero-extended compare -- the shape a screen-edge cull uses.
    hits = scan([PROLOGUE, _encode(OP_SLTIU, 4, 2, 320), EPILOGUE, JR_RA], "SYNTH+")
    cases.append(("sltiu 320 is a TEST", [h["how"].split()[0] for h in hits], ["TEST"],
                 "right edge, unsigned compare"))
    cases.append(
        ("the TEST reports the constant", hits[0]["value"] if hits else None, 320, "value, not word")
    )

    # POSITIVE: the same constant with the OPPOSITE sign-extension. A scanner handling only one form
    # would pass the control above and fail this.
    cases.append(
        ("slti 320 is a TEST", len(scan([PROLOGUE, _encode(OP_SLTI, 4, 2, 320), EPILOGUE, JR_RA])),
         1, "right edge, signed compare")
    )

    # POSITIVE: the 16.16 form, materialised by a `lui` that nothing completes. This is the control
    # that catches a mnemonic-text matcher: capstone prints `lui $v0, 0x1400` for the word that
    # builds 320<<16, so a text parse sees no frame constant at all.
    cases.append(
        ("lui 320<<16 is a BUILD",
         [h["value"] for h in scan([PROLOGUE, _encode(OP_LUI, 0, 2, 320), EPILOGUE, JR_RA])],
         [320 << 16], "16.16 screen coordinate")
    )

    # POSITIVE: a materialisation rather than a compare. `addiu rt, zero, imm` is `li`, which is how
    # an owner BUILDS the bound it later compares against in a different instruction.
    cases.append(
        ("li 320 is a BUILD", len(scan([PROLOGUE, _encode(OP_ADDIU, 0, 2, 320), EPILOGUE, JR_RA])),
         1, "literal built, not compared")
    )

    # POSITIVE: the other three frame constants, so the set is not accidentally right-edge-only.
    for value, name in ((240, "bottom edge"), (160, "centre X / half-width"), (120, "centre Y")):
        cases.append(
            (f"constant {value} is a BUILD",
             len(scan([PROLOGUE, _encode(OP_ADDIU, 0, 2, value), EPILOGUE, JR_RA])), 1, name)
        )

    # POSITIVE, and the shape that matters most: BUILD the bound into a register, then TEST it with a
    # register-register compare. This is the culling shape an owner uses when the bound cannot be an
    # immediate, and it is the shape a raw-immediate scanner structurally CANNOT see.
    built = scan(
        [PROLOGUE, _encode(OP_ADDIU, 0, 8, 320), _special(FUNCT_SLTU, 2, 8), EPILOGUE, JR_RA],
        "SYNTH+",
    )
    cases.append(("build-then-test is both", sorted(h["how"].split()[0] for h in built),
                 ["BUILD", "TEST"], "the culling shape an immediate scanner cannot see"))

    # NEGATIVE, and the false positive a raw-immediate matcher cannot avoid: the vertex colour mask
    # 0xF0F0F0F0, whose `lui` half is 0xF0 -- which is 240<<16. 96 of the 1442 raw-immediate hits in
    # the first version of this census were this one instruction pair, repeated in every area.
    # Reading the `lui` without the `ori` that completes it is what put colour masking in a cull
    # census, and a census with 96 phantom owners in it is not a census.
    mask = scan(
        [PROLOGUE, _encode(OP_LUI, 0, 15, 0xF0), _encode(OP_ORI, 15, 15, 0xF0F0),
         _special(0x24, 2, 0, 15), EPILOGUE, JR_RA],
        "SYNTH-",
    )
    cases.append(("colour constant 0x00F0F0F0 is not a hit", len(mask), 0,
                  "lui 0xF0 completed by ori -- a mask, not a screen edge"))
    mask_index = _index([PROLOGUE, _encode(OP_LUI, 0, 15, 0xF0), _encode(OP_ORI, 15, 15, 0xF0F0),
                         EPILOGUE, JR_RA], "SYNTH-", md)
    cases.append(("the lui+ori pair resolves to the mask",
                  _lui_pair(mask_index, [0, 1, 2, 3, 4], 1)[0], 0x00F0F0F0, "the completed value"))

    # NEGATIVE: a `lui` whose high half is a code address, not a bound.
    cases.append(
        ("lui 0x800f (a pointer high half) is not a hit",
         len(scan([PROLOGUE, _encode(OP_LUI, 0, 2, 0x800F), EPILOGUE, JR_RA], "SYNTH-")), 0,
         "code address, not a bound")
    )

    # NEGATIVE: non-frame constants. 512 and 1000 are real Tomba! 2 constants -- the cull's near
    # limit and the projection plane -- and neither is a frame edge, so matching them would be a
    # scanner that fires on every number in the game.
    cases.append(
        ("512 and 1000 do not match",
         len(scan([PROLOGUE, _encode(OP_SLTIU, 4, 2, 512), _encode(OP_ADDIU, 0, 3, 1000),
                   EPILOGUE, JR_RA], "SYNTH-")),
         0, "non-frame constants")
    )

    # NEGATIVE: a register-register compare where NEITHER register holds a frame constant. This is
    # the shape a DERIVED bound has, and calling it a hit would claim the engine culls against 320
    # when in fact it culls against a value it loaded.
    cases.append(
        ("register sltu with unknown operands is not a hit",
         len(scan([PROLOGUE, _special(FUNCT_SLTU, 4, 2), EPILOGUE, JR_RA], "SYNTH-")), 0,
         "derived compare, not a literal")
    )

    # POSITIVE, and the class the census must be able to NAME: a register-register compare where one
    # operand IS a frame constant. That is a literal bound compared indirectly, and it is a real cull
    # site, so it must be reported as a TEST rather than skipped as "derived".
    cases.append(
        ("register sltu against a built constant is a TEST",
         sorted(h["how"].split()[0] for h in scan(
             [PROLOGUE, _encode(OP_ADDIU, 0, 8, 320), _special(FUNCT_SLTU, 8, 9),
              EPILOGUE, JR_RA], "SYNTH+")),
         ["BUILD", "TEST"], "a literal bound compared indirectly")
    )

    # NEGATIVE: a mask that is not a frame constant. 0xF0 IS 240, so the mask here is 0x3F -- the
    # first version of this control used 0xF0 and the scanner was right to match it.
    cases.append(
        ("mask 0x3F does not match",
         len(scan([PROLOGUE, _encode(OP_ADDIU, 0, 2, 64), _encode(OP_ANDI, 4, 2, 0x3F),
                   EPILOGUE, JR_RA], "SYNTH-")),
         0, "near-miss immediate")
    )

    # STRUCTURE: trailing data past the return is not code, and is not a second function.
    index = _index([PROLOGUE, _encode(OP_ADDIU, 0, 2, 320), EPILOGUE, JR_RA, 0xAAAAAAAA], "SYNTH-", md)
    cases.append(("walk stops at jr ra", index.functions[0].size if index.functions else -1, 4,
                  "trailing data is not code"))
    cases.append(("one function found", len(index.functions), 1, "trailing word is not a prologue"))
    cases.append(("attributed <= words", len(index.attributed) <= index.image.word_count, True,
                  "coverage can never exceed the image"))

    # SCAN B, control 1: two loads carrying the SAME immediate, at different instruction addresses,
    # must be reported as the two DIFFERENT effective addresses they name. A matcher keyed on the
    # immediate rather than the effective address would report the same field twice, and would also
    # report every load of that immediate anywhere in the game.
    same_imm = 0x9B0
    same_base = DRAW_ENV_CACHE + 4 - 4 - same_imm  # base chosen so word 1 lands on +4
    found = scan_draw_env_loads(
        [_index(
            [PROLOGUE, _encode(OP_LHU, 0, 2, same_imm), _encode(OP_LHU, 0, 3, same_imm),
             EPILOGUE, JR_RA],
            "SYNTH+", md, base=same_base)]
    )
    cases.append(("same-immediate loads are both found", len(found), 2, "two distinct fields"))
    cases.append(
        ("the reported address is the EFFECTIVE one",
         sorted(h["value"] - DRAW_ENV_CACHE for h in found), [4, 8],
         "offset by the instruction address, not the immediate")
    )

    # SCAN B, control 2: the clip WIDTH and clip HEIGHT specifically. DRAWENV +4/+6 are the clip
    # w/h (game/render/wide_re_gpu_putdrawenv.cpp's own guest ABI note), so a bound read from there is
    # a bound that follows the guest's draw environment rather than a literal.
    # A load's effective address is base + 4*word + imm, so a 16-bit immediate can only name a field
    # if the base is chosen to bring CACHE into range. base = CACHE - 0x59B0 puts the first load word
    # on CACHE+4 with immediate 0x59B0, and the second on CACHE+6 with 0x59AE.
    clip_base = DRAW_ENV_CACHE - 0x59B0
    found = scan_draw_env_loads(
        [_index(
            [PROLOGUE, _encode(OP_LHU, 0, 2, 0x59B0), _encode(OP_LHU, 0, 3, 0x59AE),
             EPILOGUE, JR_RA],
            "SYNTH+", md, base=clip_base)]
    )
    cases.append(
        ("clip width and height are found",
         sorted(h["value"] - DRAW_ENV_CACHE for h in found), [4, 6],
         "DRAWENV +4 clip width, +6 clip height")
    )

    # SCAN B, negative: a load one word past the 92-byte struct is not a hit.
    past = [PROLOGUE, _encode(OP_LHU, 0, 2, 0x59B0 + DRAW_ENV_LEN), EPILOGUE, JR_RA]
    cases.append(
        ("load one word past the struct is not a hit",
         len(scan_draw_env_loads([_index(past, "SYNTH-", md, base=clip_base)])), 0,
         "the window is exactly 92 bytes")
    )

    # SCAN B, the control that catches the bug this scan had: a REGISTER-RELATIVE load cannot have
    # its effective address read off the instruction, and the first version reported it anyway. The
    # case is built so that assuming `base` is $zero lands the load squarely inside the draw-env
    # window -- so a scanner that ignores the base register calls this a hit, and the control fails.
    # A scanner that counts it as "unresolved" passes, which is the honest answer.
    deceptive = [PROLOGUE, _encode(OP_LW, 8, 2, (DRAW_ENV_CACHE + 4) & 0xFFFF), EPILOGUE, JR_RA]
    deceptive_index = _index(deceptive, "SYNTH-", md)
    got = scan_draw_env_loads([deceptive_index])
    cases.append(("register-relative load is not a draw-env hit", len(got), 0,
                  "its effective address is base+imm and the base is unknown"))
    cases.append(("it is reported as unresolved instead",
                 getattr(scan_draw_env_loads, "unresolved", None), 1,
                  "counted, not dropped and not claimed"))

    failures = 0
    for name, got, want, why in cases:
        ok = got == want
        failures += 0 if ok else 1
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}: got {got!r}, want {want!r} -- {why}")
    print(f"[selfcheck] {len(cases) - failures}/{len(cases)} controls agreed")
    return 1 if failures else 0
def _decoder() -> Cs:
    md = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN)
    md.detail = False
    return md


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--images", default=str(OVERLAYS))
    parser.add_argument("--scan", choices=["a", "b", "d", "both", "all"], default="a")
    parser.add_argument("--refs", help="hex guest address: list the in-image callers of it")
    parser.add_argument("--dis", help="hex address: disassemble the range, naming the image owner")
    parser.add_argument("--image", help="image name for --dis; required when images share a base")
    parser.add_argument("--list-images", action="store_true", help="name every image and its base")
    parser.add_argument("--to", default="0x200", help="byte count for --dis (default 0x200)")
    parser.add_argument("--context", type=int, default=8, help="instructions of context per site")
    parser.add_argument("--coverage-only", action="store_true")
    parser.add_argument("--bodies", action="store_true",
                        help="group the TEST sites by containing body and describe each shape")
    parser.add_argument("--selfcheck", action="store_true")
    args = parser.parse_args()

    if args.selfcheck:
        return selfcheck()

    images = load_images(Path(args.images))
    indexes = build_indexes(images, _decoder())

    if args.list_images:
        for index in indexes:
            print(
                f"  {index.image.name:9s} {index.image.family:8s} base {index.image.base:08X} "
                f"{len(index.image.data):7d} B"
            )
        return 0

    # Every run states the resident mapping first. A census of the wrong bytes is worse than no
    # census, and nothing else in the output would reveal it.
    resident = next(i for i in indexes if i.image.family == "resident")
    print("== resident file-offset mapping (address -> file offset A - 0x80010000 + 0x800)")
    mapping_failures = verify_resident_mapping(resident)
    if mapping_failures:
        print("[census] REFUSED: the resident mapping checks failed; the scan below would be wrong")
        return 2

    if args.dis:
        # ONE disassembly authority, and it REFUSES to guess. Every MODE image loads at the same
        # base, so "which file is this address in" is not a question a bare address can answer:
        # 0x8010BC50 is inside SOP's span AND inside A0G's, and they hold different code. A reader
        # that picks the first match gets a confident, wrong listing -- which is exactly what a
        # scratch disassembler did here. tools/overlay_owner_map.py exists for the same reason.
        start = int(args.dis, 16)
        end = start + int(args.to, 16)
        candidates = [i for i in indexes if i.image.contains(start)]
        if not candidates:
            print(f"[dis] 0x{start:08X} is in no scanned image")
            return 2
        if args.image:
            candidates = [i for i in candidates if i.image.name == args.image]
            if not candidates:
                print(
                    f"[dis] 0x{start:08X} is not in {args.image}; pass --list-images to see what is"
                )
                return 2
        elif len(candidates) > 1:
            names = ", ".join(f"{i.image.name}@{i.image.base:08X}" for i in candidates)
            print(
                f"[dis] 0x{start:08X} is in {len(candidates)} images that share this base "
                f"({names}); pass --image NAME. Refusing to guess: the same numeric address is a "
                f"different function in each."
            )
            return 2
        owner = candidates[0]
        body = owner.owner_of(start)
        print(
            f"[dis] {owner.image.name} ({owner.image.family}) base {owner.image.base:08X}"
            + (f"  body {body.start:08X} ({body.size} words)" if body else "  NOT in a walked body")
        )
        for addr in range(start - start % 4, end, 4):
            off = (addr - owner.image.base) // 4
            if not 0 <= off < len(owner.words):
                break
            print(f"  {addr:08X}  {owner.text_at(off)}")
        return 0

    if args.refs:
        target = int(args.refs, 16)
        callers = scan_callers(indexes)
        named = callers.get(target, [])
        print(f"[refs] 0x{target:08X}: {len(named)} in-image call site(s)")
        for entry in named[:64]:
            print(f"  {entry}")
        if len(named) > 64:
            print(f"  ... and {len(named) - 64} more")
        owners = [
            f"{index.image.name} body {fn.start:08X}"
            for index in indexes
            for fn in index.functions
            if fn.start == target
        ]
        if owners:
            print(f"  defined as: {'; '.join(owners)}")
        elif not named:
            print("  NOT reached: no prologue walk in these images produces this address")
        return 0

    if args.coverage_only:
        report_coverage(indexes)
        return 0

    if args.bodies:
        report_coverage(indexes)
        report_bodies(indexes, scan_frame_constants(indexes))
        return 0

    wanted = ("a", "b", "d") if args.scan in ("both", "all") else (args.scan,)
    hits: list[dict] = []
    for which in wanted:
        if which == "a":
            hits += scan_frame_constants(indexes)
        elif which == "b":
            hits += scan_draw_env_loads(indexes)
        elif which == "d":
            hits += scan_projection_writers(indexes)
    hits.sort(key=lambda h: (h["image"].name, h["addr"]))
    report_hits(indexes, hits, f"scan {args.scan}", args.context)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Refusal as refusal:
        # 77 is ctest's SKIP_RETURN_CODE. A fresh clone with no disc provisioned has no images to scan,
        # and a census that cannot scan must SKIP rather than fail -- while a PRESENT image that does
        # not authenticate is a hard failure, because that is a corrupt provision and the whole point of
        # authenticating is to catch it.
        missing = "no image at" in str(refusal) or "re-provision" in str(refusal)
        print(f"[census] {'SKIP' if missing else 'REFUSED'}: {refusal}", file=sys.stderr)
        sys.exit(77 if missing else 2)
