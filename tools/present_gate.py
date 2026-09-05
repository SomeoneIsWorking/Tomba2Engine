#!/usr/bin/env python3
"""present_gate.py — CAPTURED prims must reach the screen.

The regression gate for the most expensive and least visible bug class in this project: a producer
pushes prims, something between the push and the rasterizer discards them, and every other instrument
stays green because they all count prims that ARRIVE.

Measured 2026-08-16 (kanban #94/#35): the entire 2D panel/prompt/dialog family was missing from the
screen for weeks while the producer census reported a fully green run-end line
(`prims seen 1728103 = attributed 1708014 + unscoped-native 20089`). Four render bugs were found that
day and not one was caught by a test — all four were found by a person looking at the picture.

Each leg runs under PSXPORT_GATE_PRESENTATION=1, which makes a dropped layer ABORT rather than log
(runtime/psx/present_ledger.h).

SCENE SET — chosen so the gate CAN fail, not merely pass. `hut` and `menu` were identical across
configs for the entire period the panel bug was live, so a gate built from those alone would have
passed throughout. It must include a tier1-eligible FIELD scene (world built at present time) and a
scene whose logic frame issues TWO flushes with 2D in the first.

BOTH CONFIGS — fps60 is unified (external/psxport/docs/one-renderer.md); this is the check that keeps
it that way.

Exit: 0 all legs clean · 1 a leg dropped a layer · 2 the gate could not assert anything (refusal).
"""
import argparse
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, "scratch", "bin", "tomba2_port")

# (replay, frames, what this scene covers — why dropping it would weaken the gate)
SCENES = [
    ("bugs/cliff-fisherman-missing", 156,
     "tier1-eligible field — the world is built at present time here"),
    ("bugs/ingame-options-page", 1100,
     "START page — a 2D panel in the FIRST of the frame's two flushes"),
    ("scene-transitions/hut-entry-door-freeze", 940,
     "hut interior + the contextual prompt box"),
    ("bugs/ingame-item-menu", 1130,
     "item menu — single-flush 2D, the immune control"),
]

RECONCILED = re.compile(r"run-end: (\d+) frame\(s\) reconciled")


def run_leg(pad: str, frames: int, fps60: int, timeout: int):
    """One (scene x config) leg. Returns (ok, reconciled, detail_lines)."""
    env = dict(os.environ)
    env.update({
        "PSXPORT_GATE_PRESENTATION": "1",
        "PSXPORT_FPS60": str(fps60),
        "PSXPORT_REPL": "1",
        "PSXPORT_NO_FMV": "1",
        "PSXPORT_VK_HEADLESS": "1",
        "PSXPORT_NOPACE": "1",
        "PSXPORT_NOAUDIO": "1",
        "PSXPORT_PAD_REPLAY": os.path.join("replays", pad + ".pad"),
        "PSXPORT_SETTINGS": "psxport_settings.ini",
    })
    try:
        proc = subprocess.run([BIN], input=f"run {frames}\nquit\n", env=env, cwd=REPO,
                              capture_output=True, text=True, timeout=timeout)
        out, rc = proc.stdout + proc.stderr, proc.returncode
    except subprocess.TimeoutExpired as e:
        return False, 0, [f"TIMEOUT after {timeout}s"] + (e.stdout or "").splitlines()[-5:]

    m = RECONCILED.search(out)
    reconciled = int(m.group(1)) if m else 0
    ledger = [ln for ln in out.splitlines() if "[ledger" in ln]
    if rc != 0:
        return False, reconciled, [f"exit {rc}"] + ledger[-8:]
    # A leg that reconciled nothing proves nothing. That must FAIL, not pass quietly — it is the exact
    # failure mode ("the instrument never ran") this whole gate exists to make impossible.
    if reconciled == 0:
        return False, 0, ["0 frames reconciled — the ledger never ran, so this leg asserted NOTHING",
                          "(no run-end line found in the output)"]
    return True, reconciled, []


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--timeout", type=int, default=600, help="per-leg timeout in seconds")
    ap.add_argument("--fps60", type=int, choices=[0, 1], default=None,
                    help="run only this config (default: both, which is the point)")
    ap.add_argument("--scene", help="substring filter on the replay name")
    args = ap.parse_args()

    if not os.access(BIN, os.X_OK):
        print(f"[present-gate] REFUSED: {BIN} is not built — this gate asserted NOTHING")
        return 2
    if not os.path.isfile(os.path.join(REPO, "psxport_settings.ini")):
        print("[present-gate] REFUSED: no psxport_settings.ini")
        return 2

    configs = [args.fps60] if args.fps60 is not None else [0, 1]
    ran = skipped = failed = 0
    for pad, frames, why in SCENES:
        if args.scene and args.scene not in pad:
            continue
        if not os.path.isfile(os.path.join(REPO, "replays", pad + ".pad")):
            print(f"[present-gate] SKIP {pad} — replay MISSING, so this scene asserted nothing ({why})")
            skipped += 1
            continue
        for fps in configs:
            ok, reconciled, detail = run_leg(pad, frames, fps, args.timeout)
            ran += 1
            if ok:
                print(f"[present-gate] ok   {pad} fps60={fps} — {reconciled} frame(s) reconciled, "
                      f"no dropped layer")
            else:
                failed += 1
                print(f"[present-gate] FAIL {pad} fps60={fps}")
                for ln in detail:
                    print(f"    {ln}")

    # The denominator prints on every run, pass or fail: a green gate that ran nothing must be
    # distinguishable from a green gate that checked everything.
    print(f"[present-gate] DENOMINATOR: {ran} leg(s) run, {skipped} scene(s) skipped for a missing "
          f"replay, {failed} failed.")
    if ran == 0:
        print("[present-gate] REFUSED: no leg ran at all. This is NOT a pass.")
        return 2
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
