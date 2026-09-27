#!/usr/bin/env python3
"""Screen-edge / 4:3-width comparison census over SCUS_942.36.

WHY IT EXISTS. A widened frustum shows more world only if nothing discards it first. The
measured Tomba! 1 projection is a real horizontal FOV widening once OFX moves, so the
question this answers is not "is the frustum wide" but "is the new margin covered": a
horizontal culling owner still comparing against a retail 4:3 literal produces a
genuinely wide picture with empty sides, and `widescreen_pair.py` has a distinct verdict
(`WIDENED BUT ... MARGIN IS NOT COVERAGE`) for exactly that.

WHAT IT MEASURES, with a denominator each:

  * how many instruction words of `.text` it scanned and how many decoded;
  * how many decoded instructions carry one of the MEASURED 4:3 screen extents as a
    16-bit immediate (the literals a culling owner would hardcode);
  * how many of those are a THRESHOLD comparison (`slti`/`sltiu`/`slt`) rather than a
    plain constant use, which is the instruction shape of a cull predicate;
  * the enclosing function-relative context of every site, so a human can decompile a
    named candidate rather than trusting a grep.

It reports, never decides: a `slti $reg, $reg, 320` is a CANDIDATE, and this tool says
so. "Culling owner" is a claim about behaviour that only a decompilation can settle, and
this tool deliberately does not make it.

Usage:
    cull_census.py [--exe PATH] [--literal 320,368,384,512] [--context 4]
"""

from __future__ import annotations

import argparse
import json
import pathlib
from collections import Counter

from capstone import CS_ARCH_MIPS, CS_MODE_LITTLE_ENDIAN, CS_MODE_MIPS32, Cs

TITLE_ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = TITLE_ROOT / "executable.json"

# The measured extents. 320 is the title's recovered 4:3 draw/projection width
# (0x80016C94/0x80016CC8 SetDefDrawEnv $a3 = 0x140). 368 is the PSX 368-dot horizontal
# mode width, 384 the 15-bit half-buffer pitch, 512 the VRAM width. Each is a literal a
# horizontal culling owner could be comparing against, so each is in scope.
DEFAULT_LITERALS = (320, 368, 384, 512)

# The instruction shapes that compare a register against a threshold rather than merely
# carrying a constant. `slti`/`sltiu` take the threshold as an immediate; `slt` compares
# two registers, so a `slt` is only listed as a context line, never as a literal hit.
THRESHOLD_OPS = frozenset({"slti", "sltiu"})


class Refused(RuntimeError):
    """The scanned input cannot support the assertion the census would print."""


def load_image(path: pathlib.Path, manifest: dict) -> tuple[bytes, int, int, int]:
    if not path.is_file():
        raise Refused(
            f"no executable to scan: {path}; provision one with "
            "`uv run --frozen python titles/tomba1/tools/provision.py <disc>`"
        )
    data = path.read_bytes()
    exe = manifest.get("ps_exe")
    if not isinstance(exe, dict):
        raise Refused("manifest has no ps_exe table")
    text_address = int(exe["text_address"], 16)
    text_size = int(exe["text_size"], 16)
    header = len(data) - text_size
    if header <= 0 or text_size <= 0:
        raise Refused(f"file of {len(data)} B cannot hold a {text_size} B text section")
    if manifest.get("file_size") != len(data):
        raise Refused(
            f"executable is {len(data)} B, manifest records {manifest.get('file_size')} B"
        )
    return data, header, text_address, text_size


def immediate(insn) -> int | None:
    """The unsigned 16-bit immediate of an instruction that carries one."""
    mnemonic = insn.mnemonic
    if mnemonic in ("addiu", "ori", "slti", "sltiu", "andi", "xori", "andi16"):
        parts = [p.strip() for p in insn.op_str.split(",")]
        if not parts:
            return None
        try:
            return int(parts[-1], 0) & 0xFFFF
        except ValueError:
            return None
    if mnemonic in ("beq", "bne", "beqz", "bnez", "blez", "bgtz", "addi"):
        parts = [p.strip() for p in insn.op_str.split(",")]
        try:
            value = int(parts[-1], 0)
        except ValueError:
            return None
        return value & 0xFFFF if 0 <= value <= 0xFFFF else None
    return None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=pathlib.Path)
    parser.add_argument("--manifest", type=pathlib.Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--literal", default=",".join(str(v) for v in DEFAULT_LITERALS))
    parser.add_argument("--context", type=int, default=3)
    args = parser.parse_args(argv)

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    image = args.exe or (TITLE_ROOT.parents[1] / "scratch/bin/tomba1" / manifest["output_name"])
    data, header, base, size = load_image(image, manifest)
    wanted = {int(v, 0) for v in args.literal.split(",") if v.strip()}

    decoder = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN)
    words = size // 4
    decoded: dict[int, object] = {}
    undecodable = 0
    for index in range(words):
        address = base + index * 4
        start = header + index * 4
        got = list(decoder.disasm(data[start:start + 4], address))
        if len(got) != 1 or got[0].size != 4:
            undecodable += 1
            continue
        decoded[address] = got[0]

    hits: dict[int, list[tuple[int, object]]] = {value: [] for value in sorted(wanted)}
    for address in sorted(decoded):
        insn = decoded[address]
        value = immediate(insn)
        if value in hits:
            hits[value].append((address, insn))

    total = sum(len(sites) for sites in hits.values())
    by_op = Counter()
    for sites in hits.values():
        for _, insn in sites:
            by_op[insn.mnemonic] += 1
    thresholds = sum(
        1 for sites in hits.values() for _, insn in sites if insn.mnemonic in THRESHOLD_OPS
    )

    print(f"SCANNED: {image.name} ({len(data)} B), {manifest.get('serial')}/{manifest.get('region')}")
    print(f"SCANNED: .text 0x{base:08X}..0x{base + size:08X} = {words} instruction words, "
          f"{len(decoded)} decoded, {undecodable} undecodable (data/strings/jump tables)")
    print(f"SCANNED: {len(wanted)} measured literal(s) {sorted(wanted)}")
    print(f"MATCHED: {total} site(s) carrying a measured literal as an immediate")
    print(f"MATCHED: {thresholds} of {total} are a threshold comparison (slti/sltiu) — CANDIDATES, "
          f"not yet classified as culling owners")
    print("BY OPCODE: " + ", ".join(f"{op}={count}" for op, count in sorted(by_op.items())))
    print()
    ordered = sorted(decoded)
    for value in sorted(wanted):
        sites = hits[value]
        print(f"LITERAL {value} (0x{value:X}): {len(sites)} site(s)")
        if not sites:
            print("  0 sites (scanned every decoded instruction word)")
        for address, insn in sites:
            marker = "THRESHOLD" if insn.mnemonic in THRESHOLD_OPS else "constant"
            print(f"  {address:08X} [{marker}] {insn.mnemonic} {insn.op_str}")
            index = ordered.index(address)
            for neighbour in ordered[max(0, index - args.context):index]:
                near = decoded[neighbour]
                print(f"      ctx {neighbour:08X} {near.mnemonic} {near.op_str}")
        print()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Refused as exc:
        print(f"REFUSED: {exc}")
        raise SystemExit(2)
