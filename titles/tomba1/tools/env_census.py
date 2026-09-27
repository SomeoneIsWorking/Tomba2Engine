#!/usr/bin/env python3
"""Census of every guest site that READS Tomba! 1's own display/draw environments.

WHY IT FOLDS OVER ADDIU UNDERFLOW. `FUN_80016C4C` materializes its environment base 0x8009D6C4 as
`lui reg, 0x800A` (0x80016C6C) followed by `addiu reg, reg, 0xD6C4` (0x80016C70) — 0x800A0000 + 0xD6C4
== 0x8009D6C4, because 0x800A0000 - 0x293C also lands there. A scanner that splits the address on its
written digits finds nothing and reports a clean zero, which is the worst possible outcome: it looks
like a measurement and is an artifact of the search. This one enumerates, for every `lui` site, the
set of `addiu` sites that complete a materialization of the target under BOTH the written and the
underflow form.

WHAT IT IS FOR. The title's widening replaced DRAWENV RECT.w (320 -> 428) and the running product's
own clear changed SHAPE — from a GP0(02) VRAM fill to a GP0(60) screen rect — which is a behavioural
difference, not merely a wider picture. This census names every site that reads those structures, so
the decision can be located in the guest rather than guessed at.

Usage:
    env_census.py [--exe PATH] [--target 0x8009D6C4 ...]
"""

from __future__ import annotations

import argparse
import json
import pathlib
from collections import defaultdict

from capstone import CS_ARCH_MIPS, CS_MODE_LITTLE_ENDIAN, CS_MODE_MIPS32, Cs

TITLE_ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = TITLE_ROOT / "executable.json"

# The three structures the frame loop depends on, and the two the widening touches.
DEFAULT_TARGETS = (0x8009D6B0, 0x8009D6C4, 0x8009E3C0, 0x8009E3D4)


class Refused(RuntimeError):
    """The scan cannot be run, and saying so beats printing an empty result."""


def load(path: pathlib.Path, manifest: dict) -> tuple[bytes, int, int, int]:
    if not path.is_file():
        raise Refused(
            f"no executable to scan: {path}; provision one with "
            "`uv run --frozen python titles/tomba1/tools/provision.py <disc>`"
        )
    data = path.read_bytes()
    exe = manifest["ps_exe"]
    text_address = int(exe["text_address"], 16)
    text_size = int(exe["text_size"], 16)
    header = len(data) - text_size
    if manifest.get("file_size") != len(data):
        raise Refused(f"executable is {len(data)} B, manifest records {manifest.get('file_size')} B")
    return data, header, text_address, text_size


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=pathlib.Path)
    parser.add_argument("--manifest", type=pathlib.Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--target", default=",".join(f"0x{value:08X}" for value in DEFAULT_TARGETS))
    args = parser.parse_args(argv)

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    image = args.exe or (TITLE_ROOT.parents[1] / "scratch/bin/tomba1" / manifest["output_name"])
    data, header, base, size = load(image, manifest)
    targets: dict[int, str] = {}
    for value in args.target.split(","):
        if value.strip():
            address = int(value, 16)
            targets[address] = f"0x{address:08X}"

    decoder = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN)
    words = size // 4
    decoded: dict[int, object] = {}
    undecodable = 0
    for index in range(words):
        address = base + index * 4
        chunk = data[header + index * 4:header + index * 4 + 4]
        got = list(decoder.disasm(chunk, address))
        if len(got) != 1 or got[0].size != 4:
            undecodable += 1
            continue
        decoded[address] = got[0]

    # For each register, remember the LUI that last set its upper half, and let a following ADDIU on
    # the SAME register complete the pair. The underflow form is a distinct accepted completion, not
    # a second pass: 0x800A0000 + (-0x293C) is the same address written differently.
    pending: dict[str, tuple[int, int]] = {}
    hits: dict[int, list[tuple[int, int]]] = defaultdict(list)
    for address in sorted(decoded):
        insn = decoded[address]
        parts = [part.strip() for part in insn.op_str.split(",")]
        if insn.mnemonic == "lui" and len(parts) == 2:
            try:
                pending[parts[0]] = (address, int(parts[1], 0) & 0xFFFF)
            except ValueError:
                pending.pop(parts[0], None)
            continue
        if insn.mnemonic == "addiu" and len(parts) == 3 and parts[0] == parts[1]:
            lui = pending.get(parts[0])
            if lui is not None:
                try:
                    low = int(parts[2], 0)
                except ValueError:
                    low = None
                if low is not None:
                    # Sign-extended 16-bit: a negative immediate is a subtracted address.
                    value = ((lui[1] << 16) + (low & 0xFFFF if low >= 0 else low)) & 0xFFFFFFFF
                    if value in targets:
                        hits[value].append((lui[0], address))
            pending.pop(parts[0], None)
            continue
        if insn.mnemonic not in ("addiu", "ori", "li"):
            pending.pop(parts[0] if parts else "", None)

    total = sum(len(sites) for sites in hits.values())
    print(f"SCANNED: {image.name} ({len(data)} B), {manifest.get('serial')}/{manifest.get('region')}")
    print(f"SCANNED: .text 0x{base:08X}..0x{base + size:08X} = {words} words, {len(decoded)} decoded, "
          f"{undecodable} undecodable")
    print(f"SCANNED: {len(targets)} target(s), matched over the WRITTEN and the UNDERFLOW addiu form")
    print(f"MATCHED: {total} materializing site pair(s) of {len(targets)} target(s)")
    for target, name in targets.items():
        sites = hits.get(target, [])
        print(f"{name}: {len(sites)} site(s)")
        for lui, addiu in sites:
            print(f"    lui 0x{lui:08X}  +  addiu 0x{addiu:08X}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Refused as exc:
        print(f"REFUSED: {exc}")
        raise SystemExit(2)
