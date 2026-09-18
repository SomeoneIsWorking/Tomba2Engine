#!/usr/bin/env python3
"""Compare the Tomba! 2 Lightrec product against psxport's independent Beetle full-console
reference at title-owned state checkpoints (psxport docs/oracle.md).

Both cores boot the same disc. Each is driven toward the next checkpoint by the same input policy
(oracle_tomba2.py); the first arrival parks; once both arrive the declared RAM ranges are compared
and every excluded field is named. After the free-roam checkpoint both cores receive identical held
inputs for identical game-frame counts and are compared after each segment (or every --frame-step
frames). The report names both cores'
frame counts, every range with its byte count, the first differing byte, and the identities of the
binary, disc, firmware and reference build.

    uv run --frozen python tools/oracle_compare.py --bios ../SCPH1001.BIN
    uv run --frozen python tools/oracle_compare.py --bios ../SCPH1001.BIN --selftest

`--selftest` seeds one byte of the product's player position at the first checkpoint and requires
the comparator to report exactly that divergence; a comparator that has never shown a difference is
not trusted (docs/oracle.md evidence rules).
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import gate  # noqa: E402  (tools/gate.py owns the product launch environment and binary identity)
import oracle_tomba2 as title  # noqa: E402
from oracle_cores import ConsoleSession, CoreError, NativeReplSession  # noqa: E402

REPO = Path(gate.REPO)
PSXPORT = Path(gate.PSXPORT)
OUT_DIR = REPO / "scratch" / "oracle"
DEFAULT_BIOS = REPO.parent / "SCPH1001.BIN"


def compare(native: dict[str, bytes], console: dict[str, bytes]) -> list[dict]:
    """One row per declared range: byte count and the first differing offset, if any."""
    rows = []
    for declared in title.DECLARED:
        a, b = native[declared.name], console[declared.name]
        first = next((i for i in range(declared.size) if a[i] != b[i]), None)
        row = {"range": declared.name, "address": f"0x{declared.address:08X}", "bytes": declared.size,
               "decisive": declared.decisive, "equal": first is None,
               "native_hex": a.hex(), "console_hex": b.hex()}
        if first is not None:
            row.update({"first_diff_offset": first, "native_byte": a[first], "console_byte": b[first],
                        "differing_bytes": sum(1 for i in range(declared.size) if a[i] != b[i])})
        rows.append(row)
    return rows


def checkpoint(name: str, native, console, report: dict) -> bool:
    """Snapshot both parked cores, compare, record; return whether every decisive range matched."""
    rows = compare(title.snapshot(native), title.snapshot(console))
    sm_native = title.observe(native).sm
    sm_console = title.observe(console).sm
    entry = {"checkpoint": name, "native_frames": native.frames, "console_vblanks": console.frames,
             "native_sm": sm_native, "console_sm": sm_console, "ranges": rows}
    report["checkpoints"].append(entry)
    ok = all(row["equal"] for row in rows if row["decisive"])
    verdict = "MATCH" if ok else "DIVERGE"
    print(f"[oracle] {name}: {verdict} native f{native.frames} sm{sm_native} | "
          f"console vb{console.frames} sm{sm_console}")
    for row in rows:
        if not row["equal"]:
            kind = "decisive" if row["decisive"] else "informational"
            print(f"[oracle]   {row['range']} ({kind}): first diff at +{row['first_diff_offset']} "
                  f"native {row['native_byte']:02X} console {row['console_byte']:02X}, "
                  f"{row['differing_bytes']}/{row['bytes']} bytes differ"
                  + (f" [native {row['native_hex']} console {row['console_hex']}]" if row["decisive"] else ""))
    return ok


def selftest(native, console, report: dict) -> int:
    """Seed one player-position byte on the product and require exactly that range to diverge."""
    address = title.PLAYER_G + 0x2C
    original = native.read(address, 1)[0]
    native.write8(address, original ^ 0x5A)
    rows = compare(title.snapshot(native), title.snapshot(console))
    seeded = {row["range"]: row for row in rows}
    position = seeded["player.position"]
    hit = (not position["equal"]) and position["first_diff_offset"] == 0
    report["selftest"] = {"seeded_address": f"0x{address:08X}", "detected": hit, "ranges": rows}
    print(f"[oracle] selftest: seeded byte at 0x{address:08X}; comparator "
          f"{'DETECTED it' if hit else 'MISSED it'}")
    return 0 if hit else 1


def gameplay(native, console, report: dict, frame_step: int) -> bool:
    """Feed both cores the title's held-input segments frame by frame (title.Playback owns each
    core's delivery timing) and compare after every `frame_step` frames of a segment (once per
    segment when 0); stop at the first decisive divergence."""
    schedule = [buttons for buttons, frames in title.GAMEPLAY for _ in range(frames)]
    players = [title.Playback(core, schedule) for core in (native, console)]
    for index, (buttons, frames) in enumerate(title.GAMEPLAY):
        for done in range(1, frames + 1):
            for player in players:
                player.step()
            if done == frames or (frame_step > 0 and done % frame_step == 0):
                label = f"gameplay[{index}] hold {sorted(buttons) or 'nothing'} {done}/{frames}f"
                if not checkpoint(label, native, console, report):
                    return False
    return True


def run(args) -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    if not args.bios.is_file():
        print(f"REFUSED: BIOS image {args.bios} does not exist; pass --bios <SCPH1001.BIN>", file=sys.stderr)
        return 2
    if not os.path.isfile(gate.BIN) or not os.path.isfile(gate.EXE):
        print(f"REFUSED: {gate.BIN} or {gate.EXE} is missing; build the product first", file=sys.stderr)
        return 2
    disc = Path(gate.native_environment(args.watchdog)["PSXPORT_TOMBA2_DISC"])
    report = {"binary": gate._binary_identity(), "disc": str(disc), "bios": str(args.bios),
              "declared": [{"range": d.name, "address": f"0x{d.address:08X}", "bytes": d.size,
                            "decisive": d.decisive} for d in title.DECLARED],
              "excluded": title.EXCLUDED, "checkpoints": [], "complete": False}
    started = time.monotonic()
    native = console = None
    exit_code = 1
    try:
        native = NativeReplSession(gate.BIN, gate.EXE, gate.native_environment(args.watchdog),
                                   gate.REPO, OUT_DIR / "native.log")
        console = ConsoleSession(PSXPORT, disc, args.bios, args.region, OUT_DIR / "console.log")
        report["console_manifest"] = console.manifest
        # The console is driven first at every checkpoint: its arrival frame fixes the settle pad
        # the product's arrival frame must read (title._drive).
        settle = None
        for core in (console, native):
            used, settle = title.reach_game(core, args.budget, settle)
            print(f"[oracle] {core.name}: GAME stage after {used} game frames")
        ok = checkpoint("game_stage", native, console, report)
        if args.selftest:
            exit_code = selftest(native, console, report)
            report["complete"] = True
            return exit_code
        settle = None
        for core in (console, native):
            used, settle = title.reach_field(core, args.budget, settle)
            print(f"[oracle] {core.name}: field after {used} more game frames")
        ok = checkpoint("field", native, console, report) and ok
        settle = None
        for core in (console, native):
            used, settle = title.reach_free_roam(core, args.budget, settle)
            print(f"[oracle] {core.name}: free roam after {used} more game frames")
        ok = checkpoint("free_roam", native, console, report) and ok
        ok = gameplay(native, console, report, args.frame_step) and ok
        report["complete"] = True
        exit_code = 0 if ok else 1
        return exit_code
    except CoreError as error:
        report["error"] = str(error)
        print(f"[oracle] FAILED: {error}", file=sys.stderr)
        return 1
    finally:
        for core in (native, console):
            if core is not None:
                core.close()
        report["seconds"] = round(time.monotonic() - started, 1)
        path = OUT_DIR / ("selftest.json" if args.selftest else "compare.json")
        path.write_text(json.dumps(report, indent=2))
        print(f"[oracle] report: {path} ({report['seconds']}s)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--bios", type=Path, default=DEFAULT_BIOS,
                        help=f"authentic NTSC-U BIOS image (default {DEFAULT_BIOS})")
    parser.add_argument("--region", default="na")
    parser.add_argument("--budget", type=int, default=6000, help="frame budget per checkpoint per core")
    parser.add_argument("--watchdog", type=int, default=3600, help="product watchdog seconds")
    parser.add_argument("--frame-step", type=int, default=0,
                        help="compare every N frames inside a gameplay segment (0 = once per segment)")
    parser.add_argument("--selftest", action="store_true", help="validate the comparator with a seeded divergence")
    return run(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
