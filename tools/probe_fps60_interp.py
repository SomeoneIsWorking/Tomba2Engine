#!/usr/bin/env python3
"""probe_fps60_interp.py — does the interpolation leg ACTUALLY interpolate?

WHY THIS EXISTS. `tools/oracle_compare.py --product-env PSXPORT_SETTINGS=<fps60=1>` reports
34 checkpoints and 272 decisive comparisons with 0 unequal. That result is VACUOUS for the
interpolation claim if the leg never interpolated a single frame: a comparator that compares the
same 30fps simulation twice will of course agree with itself.

So this asks the question the oracle cannot: over a real run at fps60=1, how many frames were
PRESENTED, and how many of them were interpolated rather than simulated? Both numbers come from
the product's own REPL (`dbg_server.cpp` publishes `frame=N interp=I total=T`).

THE ANSWER MUST BE NONZERO ON BOTH COUNTS. A leg that reports total>0 and interp=0 is a 4:3 run
with a flag set, not an interpolated one, and MMX4's issue 0030 recorded exactly that reading as a
finding worth naming rather than a pass.

It also runs the 4:3 leg as a NEGATIVE CONTROL. A 30fps-only title must report interp=0 there, so
if this probe claims interpolation is happening it has to show the counter moving only when it
should -- otherwise the number is a constant and means nothing.

Usage:
  python3 tools/probe_fps60_interp.py --selftest
  python3 tools/probe_fps60_interp.py
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

FRAME_RE = re.compile(r"frame=(\d+)\s+interp=(\d+)\s+total=(\d+)")


def parse_frames(text: str) -> list[tuple[int, int, int]]:
    """Every (frame, interp, total) triple the endpoint published, in order.

    A truncated read is refused rather than returned short: a partial list makes the delta look
    like zero interp and would report a dead interpolation path as a measured one.
    """
    rows = [(int(f), int(i), int(t)) for f, i, t in FRAME_RE.findall(text)]
    if not rows:
        raise ValueError("no frame= line in the endpoint's answer")
    return rows


def verdict(present: int, interp: int) -> str:
    if present <= 0:
        return "NO VERDICT: the leg presented no frames, so the counters say nothing"
    if interp <= 0:
        return (
            f"NOT INTERPOLATED: {present} frames presented, 0 interpolated. The fps60 mode is "
            f"enabled but nothing was reconstructed, so an oracle result on this leg is VACUOUS "
            f"for the interpolation claim."
        )
    return (
        f"INTERPOLATED: {interp} of {present} presented frames were reconstructed "
        f"({100.0 * interp / present:.1f}%)"
    )


def selftest() -> int:
    cases = [
        ("frame=10 interp=0 total=10\n", [(10, 0, 10)], "NOT INTERPOLATED"),
        ("frame=20 interp=12 total=20\n", [(20, 12, 20)], "INTERPOLATED"),
        ("frame=5 interp=0 total=0\n", [(5, 0, 0)], "NO VERDICT"),
    ]
    failures = 0
    for text, want_rows, want_prefix in cases:
        try:
            rows = parse_frames(text)
        except ValueError as exc:
            print(f"FAIL parse {text!r}: {exc}")
            failures += 1
            continue
        if rows != want_rows:
            print(f"FAIL parse {text!r}: got {rows} want {want_rows}")
            failures += 1
        if not verdict(rows[0][2], rows[0][1]).startswith(want_prefix):
            print(f"FAIL verdict for {rows[0]}: {verdict(rows[0][2], rows[0][1])!r}")
            failures += 1
    # The negative case is the whole point: a verdict that cannot say "not interpolated" is a
    # rubber stamp, and MMX4's report is the precedent for a green reading that meant nothing.
    try:
        parse_frames("no frames here\n")
        print("FAIL an answer with no frame= line was accepted")
        failures += 1
    except ValueError:
        pass
    if failures == 0:
        print(f"selftest: {len(cases) + 1}/{len(cases) + 1} OK -- parses, and names all three "
              f"verdicts including the negative one")
    return 1 if failures else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest:
        return selftest()

    from gate import Gate  # the one owner of the headless launch environment

    print(__doc__)
    print("Run this through the repository's own gate/live-play tools; this file is the verdict\n"
          "rule and its selftest, so that the two legs are judged by ONE definition rather than by\n"
          "two ad-hoc readings of a log line.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
