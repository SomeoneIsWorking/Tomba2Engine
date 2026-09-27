#!/usr/bin/env python3
"""Exhaustive reachability census over SCUS_942.36 for the guest projection owners.

This is the negative-first diagnostic for Tomba! 1's widescreen work. It reports what it
SCANNED and what it MATCHED, refuses a missing or wrong-size executable, and never
infers an answer from a heuristic: every match is a decoded MIPS instruction whose
target is the named address.

What it measures, with a denominator each:

  * the size and identity of the scanned `.text` (from `executable.json`);
  * every instruction word in `.text` that can reach a named guest address as a
    direct `jal` (the only way a `jal`-targeted leaf is reached in this ABI, since
    neither leaf is address-taken — see the `pointer` section);
  * the same census over every `lui`/`addiu` (or `lui`/`ori`) pair that MATERIALIZES
    one of those addresses in a general register, which is how a `jalr $t9` reaches
    one;
  * the immediate argument each direct caller passes in `$a0`, decoded from the
    instruction in the branch delay slot, with the non-delay-slot case reported
    separately rather than guessed.

Usage:
    scan_projection.py [--exe PATH] [--callers 0x80063A34,0x80063A54]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

from capstone import CS_ARCH_MIPS, CS_MODE_LITTLE_ENDIAN, CS_MODE_MIPS32, Cs

TITLE_ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = TITLE_ROOT / "executable.json"

# MIPS register numbers used by this census.
_REG_A0 = 4
_REG_RA = 31
_REG_NAMES = (
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",
)


class Refused(RuntimeError):
    """The scanned input cannot support the assertion the census would print."""


def load_manifest(path: pathlib.Path) -> dict:
    if not path.is_file():
        raise Refused(f"executable manifest is missing: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def load_executable(path: pathlib.Path, manifest: dict) -> bytes:
    if not path.is_file():
        raise Refused(f"no executable to scan: {path}")
    data = path.read_bytes()
    exe = manifest.get("ps_exe")
    if not isinstance(exe, dict):
        raise Refused("manifest has no ps_exe table")
    text_address = int(exe["text_address"], 16)
    text_size = int(exe["text_size"], 16)
    header = len(data) - text_size
    if header <= 0 or text_size <= 0:
        raise Refused(
            f"file of {len(data)} B cannot hold a {text_size} B text section behind a header"
        )
    if manifest.get("file_size") != len(data):
        raise Refused(
            f"executable is {len(data)} B, manifest records {manifest.get('file_size')} B"
        )
    manifest.setdefault("_scan", {})["header"] = header
    manifest["_scan"]["text_address"] = text_address
    manifest["_scan"]["text_size"] = text_size
    return data


class Text:
    """The `.text` window plus the address<->offset mapping the census scans."""

    def __init__(self, data: bytes, header: int, base: int, size: int) -> None:
        self._data = data
        self._header = header
        self.base = base
        self.size = size
        self.end = base + size

    def contains(self, address: int) -> bool:
        return self.base <= address < self.end

    def raw(self, address: int, length: int = 4) -> int:
        offset = self._header + (address - self.base)
        return int.from_bytes(self._data[offset:offset + length], "little")

    def word_at(self, address: int) -> int:
        return self.raw(address)

    def slice(self, lo: int, hi: int) -> bytes:
        start = self._header + (lo - self.base)
        return self._data[start:self._header + (hi - self.base)]


def immediate_register(insn) -> tuple[str, int] | None:
    """A decoded `li`-shaped materialization, as (destination, unsigned value)."""
    mnemonic = insn.mnemonic
    if mnemonic in ("addiu", "ori"):
        parts = [p.strip() for p in insn.op_str.split(",")]
        if len(parts) != 3 or parts[0].startswith("$0x") or parts[0].startswith("$zero"):
            return None
        try:
            return parts[0], int(parts[2], 0) & 0xFFFF
        except ValueError:
            return None
    if mnemonic == "li":
        parts = [p.strip() for p in insn.op_str.split(",")]
        if len(parts) != 2:
            return None
        try:
            return parts[0], int(parts[1], 0) & 0xFFFF
        except ValueError:
            return None
    return None


def is_zero_source(text: Text, insn) -> bool:
    """True when the instruction's 16-bit immediate is loaded from the zero register.

    The projection arguments are small positive constants, so `li $a0, 0x220` is
    `addiu $a0, $zero, 0x220` (opcode 0x24, `rs == 0`) and `addiu $a0, $zero, -N`
    is opcode 0x24 with a sign-extended negative immediate. Reading the operand text
    for `$zero` covers both without re-implementing the decoder.
    """
    return "$zero" in insn.op_str or insn.op_str.startswith("0")


class Caller:
    def __init__(self, site: int, kind: str, argument: str) -> None:
        self.site = site
        self.kind = kind
        self.argument = argument


def scan(text: Text, targets: dict[int, str], decoder: Cs) -> dict:
    """Scan every instruction word in `.text` and report what reached each target."""
    words = text.size // 4
    report: dict[int, dict] = {
        address: {
            "name": name,
            "direct_callers": [],
            "register_callers": [],
            "pointer_words": [],
        }
        for address, name in targets.items()
    }
    # Reachability tallies. A diagnostic that prints a match without saying how much
    # it looked at has not shown that it scanned anything.
    scanned = 0
    undecodable = 0
    # `lui reg, hi` immediately followed by `addiu reg, reg, lo` materializes a
    # 32-bit constant; keep the last `lui` destination and its upper half.
    pending_lui: tuple[str, int] | None = None

    for index in range(words):
        address = text.base + index * 4
        if address % 4:
            continue
        chunk = text.slice(address, address + 4)
        decoded = list(decoder.disasm(chunk, address))
        if len(decoded) != 1 or decoded[0].size != 4:
            undecodable += 1
            pending_lui = None
            continue
        insn = decoded[0]
        scanned += 1

        if insn.mnemonic == "lui":
            parts = [p.strip() for p in insn.op_str.split(",")]
            try:
                pending_lui = (parts[0], int(parts[1], 0) << 16)
            except (IndexError, ValueError):
                pending_lui = None
        else:
            materialized = immediate_register(insn)
            if materialized is not None and pending_lui is not None:
                destination, low = materialized
                if destination == pending_lui[0]:
                    value = (pending_lui[1] + low) & 0xFFFFFFFF
                    if value in targets:
                        report[value]["register_callers"].append((address, destination))
            if insn.mnemonic != "lui":
                pending_lui = None

        if insn.mnemonic == "jal":
            parts = [p.strip() for p in insn.op_str.split(",")]
            try:
                called = int(parts[0], 0)
            except (IndexError, ValueError):
                continue
            if called in targets:
                report[called]["direct_callers"].append(address)

        # A raw constant equal to a target address is a data pointer or a jump-table
        # entry, and is the only way these two non-address-taken leaves could be
        # reached by `jalr`. Reported so the count is honest about that path.
        word = text.word_at(address)
        for target in targets:
            if word == target:
                report[target]["pointer_words"].append(address)

    report["_scan"] = {
        "words": words,
        "decoded": scanned,
        "undecodable": undecodable,
        "text_lo": text.base,
        "text_hi": text.end,
    }
    return report


def caller_argument(text: Text, decoder: Cs, site: int) -> tuple[str, str]:
    """Decode the `$a0` argument a direct caller passes, from its delay slot.

    Returns (value_text, provenance). A caller that does not materialize `$a0` in its
    own delay slot is reported as `register` rather than guessed: the census states
    what it read and stops there.
    """
    delay = site + 4
    if not text.contains(delay):
        return "?", "delay slot outside .text"
    decoded = list(decoder.disasm(text.slice(delay, delay + 4), delay))
    if len(decoded) != 1 or decoded[0].size != 4:
        return "?", "delay slot did not decode"
    insn = decoded[0]
    materialized = immediate_register(insn)
    if materialized is None:
        return "?", f"delay slot is `{insn.mnemonic} {insn.op_str}`"
    destination, raw = materialized
    if destination not in ("$a0", "$4"):
        return "?", f"delay slot writes {destination}, not $a0"
    if raw >= 0x8000:
        value = raw - 0x10000
    else:
        value = raw
    return f"0x{value:X} ({value})", "delay-slot addiu"


def text_range_for(site: int, text: Text, decoder: Cs) -> tuple[int, int]:
    """A narrow, honest function window: the call site plus its immediate prologue."""
    lo = max(text.base, (site // 4) * 4)
    return lo, min(text.end, site + 0x40)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=pathlib.Path)
    parser.add_argument("--manifest", type=pathlib.Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--callers", default="0x80063A34,0x80063A54")
    args = parser.parse_args(argv)

    manifest = load_manifest(args.manifest)
    candidates = []
    if args.exe:
        candidates.append(args.exe)
    else:
        candidates.append(TITLE_ROOT.parents[1] / "scratch/bin/tomba1" / manifest["output_name"])
    image = next((p for p in candidates if p.is_file()), None)
    if image is None:
        raise Refused(
            "no scanned executable; provision one with "
            f"`uv run --frozen python titles/tomba1/tools/provision.py <disc>` (tried: "
            + ", ".join(str(p) for p in candidates)
            + ")"
        )
    data = load_executable(image, manifest)
    scan_info = manifest["_scan"]
    text = Text(data, scan_info["header"], scan_info["text_address"], scan_info["text_size"])

    targets: dict[int, str] = {}
    for item in args.callers.split(","):
        item = item.strip()
        if not item:
            continue
        address = int(item, 16)
        if not text.contains(address):
            raise Refused(f"census target 0x{address:08X} lies outside the scanned .text")
        targets[address] = f"guest 0x{address:08X}"

    decoder = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN)
    report = scan(text, targets, decoder)
    counters = report.pop("_scan")

    print(f"SCANNED: executable {image.name} ({len(data)} B), manifest identity "
          f"{manifest.get('serial')}/{manifest.get('region')}")
    print(f"SCANNED: .text 0x{counters['text_lo']:08X}..0x{counters['text_hi']:08X} = "
          f"{counters['words']} instruction words, {counters['decoded']} decoded, "
          f"{counters['undecodable']} undecodable")
    for address in sorted(targets):
        entry = report[address]
        total = (len(entry["direct_callers"]) + len(entry["register_callers"])
                 + len(entry["pointer_words"]))
        print()
        print(f"TARGET: {entry['name']}  matched {total} site(s) of 3 reference kinds")
        if not entry["direct_callers"]:
            print("  jal: 0 (scanned every instruction word)")
        for site in entry["direct_callers"]:
            argument, provenance = caller_argument(text, decoder, site)
            print(f"  jal:  0x{site:08X}  $a0 = {argument}  [{provenance}]")
        for site, register in entry["register_callers"]:
            print(f"  reg:  0x{site:08X}  materialized into {register} (jalr-reachable)")
        if entry["pointer_words"]:
            shown = ", ".join(f"0x{a:08X}" for a in entry["pointer_words"][:8])
            print(f"  ptr:  {len(entry['pointer_words'])} raw constant(s): {shown}")
        else:
            print("  ptr:  0 raw constants equal the address (no function pointer to it)")
    print()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Refused as exc:
        print(f"REFUSED: {exc}")
        raise SystemExit(2)
