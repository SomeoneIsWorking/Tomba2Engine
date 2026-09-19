#!/usr/bin/env python3
"""Does Tomba! 2's widescreen EXTEND the picture, or change it?

    uv run --frozen python tools/widescreen_check.py

Drives the product to one settled free-roam state twice -- once at 4:3, once at 16:9 -- and hands
the two captures to psxport's widescreen analyser, which owns the question because it is
title-neutral. This tool owns only how Tomba! 2 reaches a comparable state.

WHY THE STATE ORACLE DOES NOT ANSWER THIS. tools/oracle_compare.py reports 34/34 checkpoints
byte-identical with widescreen on, and would report exactly that for a port which stretched its 4:3
frame to fill the wider viewport: stretching writes no guest state. The picture oracle cannot answer
it either, because its reference is a console and a console is 4:3 -- there is nothing to compare
the extra area against. So the product is asked about itself.
"""

from __future__ import annotations

import json
import os
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
REPO = TOOLS.parent
sys.path.insert(0, str(TOOLS))
sys.path.insert(0, str(REPO / "external" / "psxport" / "tools" / "oracle"))

import gate  # noqa: E402  (tools/gate.py owns the product launch environment)
import widescreen  # noqa: E402

OUT_DIR = REPO / "scratch" / "widescreen"
# Settled free roam: past the prologue and past the area fade, with no input held, so the two runs
# render one scene rather than two moments of a fade. The frame count is the picture oracle's
# free_roam checkpoint plus the settling its advance-to-scene step measured (docs/issues/0020).
SETTLE_FRAMES = 240
ASPECTS = {"narrow": "aspect=0\nfps60=0\n", "wide": "aspect=1\nfps60=0\n"}


def capture(name: str, settings_body: str) -> Path:
    settings = OUT_DIR / f"{name}.ini"
    settings.write_text(settings_body)
    shot = OUT_DIR / f"{name}.png"
    if shot.exists():
        shot.unlink()
    script = f"newgame\nrun {SETTLE_FRAMES}\nshot {shot}\nquit\n"
    code = gate.run_gate(script, SETTLE_FRAMES, '', 240, 0, {}, f"widescreen-{name}",
                         settings=str(settings))
    if code != 0:
        sys.exit(f"REFUSED: the {name} run failed ({code}); nothing was measured")
    if not shot.is_file():
        sys.exit(f"REFUSED: the {name} run reported success but wrote no capture at {shot}")
    return shot


def announced_geometry(name: str) -> str:
    """The product's own statement of what it rendered. A run whose aspect failed to engage looks
    exactly like one that engaged (docs/issues/0130 in the Spyro tree), so this is not optional."""
    logs = sorted(Path(gate.LOGDIR).glob(f"gate-widescreen-{name}-*.log"))
    if not logs:
        sys.exit(f"REFUSED: no log for the {name} run, so its picture geometry is unknown")
    lines = [line for line in logs[-1].read_text(errors="replace").splitlines()
             if "[wide] native picture:" in line]
    if not lines:
        sys.exit(f"REFUSED: the {name} run never announced its picture geometry, so there is no "
                 f"evidence it rendered at the requested aspect")
    return lines[-1].split("[wide] ")[-1]


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    captures = {}
    for name, body in ASPECTS.items():
        captures[name] = capture(name, body)
        print(f"[widescreen] {name}: {announced_geometry(name)}")
    try:
        result = widescreen.analyse(captures["narrow"], captures["wide"], OUT_DIR)
    except widescreen.Unanswerable as refusal:
        print(f"[widescreen] REFUSED: {refusal}", file=sys.stderr)
        return 2
    widescreen.announce(result)
    report = OUT_DIR / "widescreen.json"
    report.write_text(json.dumps(result.report(), indent=2))
    print(f"[widescreen] report: {report}")
    print("[widescreen] NEITHER NUMBER SAYS THE EXTRA GEOMETRY IS CORRECT — no 16:9 reference "
          "exists to say that.")
    return 0 if result.extends else 1


if __name__ == "__main__":
    raise SystemExit(main())
