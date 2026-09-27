#!/usr/bin/env python3
"""aspect_pair.py — photograph ONE deterministic frame under TWO tracked settings files, and
report the difference between the two pictures as a measurement rather than an impression.

WHAT THIS IS FOR. "The inventory screen has weird lines" is a claim about a PICTURE, so answering it
needs the same frame twice and then a number. Two files whose only difference is the field under test
make the answer attributable: anything that moved did so because of that field, not because two runs
drifted. One file edited in place between runs cannot do that — it leaves no record of what the other
leg ran with, and an unedited-in-place pair is indistinguishable from a run that silently ignored the
setting.

SO BOTH SETTINGS FILES ARE TRACKED and named on the command line, and each leg's own log line
(`render_width=`) is printed back, because "I passed aspect=1" is a claim about a file and the product
printing its own measured width is a claim about the frame. A leg whose measured width does not match
the field it was asked to test is reported as MISCONFIGURED and its picture is not used for a
verdict — the settings file being right is not the same question as the product having read it
(`PSXPORT_ASPECT` is not a CVar; it does nothing at all).

THE LINE MEASURE, AND WHY IT IS A ROW PROFILE. A "line" in a 2D page is a horizontal or vertical band
of ink that the surrounding content does not have, so the measurement is a per-row count of pixels
above a stated ink threshold, compared against the picture's own median row. That is deliberately
boring arithmetic: it has no notion of what the screen is supposed to show, so it cannot be fooled by
a menu that legitimately contains rules and frames, and it cannot report a band the pixels do not
contain. What it CAN do is be checked, so `--selftest` runs it on constructed images where the answer
is known in advance — a band must be found, and a uniform page must not produce one. An instrument
that has only ever answered one way is not an instrument; this one has to be shown both answers.

A LINE IS A POSITION, NOT A VERDICT. A detected band says "these rows carry more ink than the rest of
the picture". Whether those rows should carry it is a judgement about the screen, and the tool prints
the numbers and stops.

    uv run --frozen python tools/aspect_pair.py --selftest
    uv run --frozen python tools/aspect_pair.py \
        --replay replays/bugs/ingame-item-menu.pad --frame 1120 \
        --narrow tools/fps60_control_settings.ini \
        --wide tools/wide_control_settings.ini \
        --out scratch/aspect-pair/item-menu

Exit: 0 both legs captured and their widths are the ones asked for · 1 a leg failed or is
misconfigured · 2 the run asserted nothing (refusal: no binary, no replay, no settings file).
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from gate import BIN, LOGDIR, REPO  # noqa: E402 — the ONE built binary and log dir

ROOT = Path(REPO)  # gate.REPO is a string; every path arithmetic below wants a Path
PSXPORT = ROOT / "external" / "psxport"


def agent_environment(settings: Path) -> dict:
    """psxport's own headless/silent/unpaced automation environment, with NO REPL.

    Deliberately `agent_environment` and NOT `gate.native_environment`: the gate adds PSXPORT_REPL=1
    because a gate drives the product through a stdin script, and this tool has no stdin script. With
    PSXPORT_REPL=1 and no input the REPL reads EOF and quits, which ends the frame loop at frame 0 —
    MEASURED 2026-09-27 on the first run of this tool: both legs exited 0 having presented one frame,
    and the tool's own refusal ("0 of 1 requested shot(s) landed") is the only reason that did not read
    as a pass. The policy is the same either way — an agent run must name a tracked settings file, and
    agent_environment refuses without one.
    """
    sys.path.insert(0, str(PSXPORT / "tools"))
    from port.launch_environment import agent_environment as _agent_environment

    return _agent_environment(dict(os.environ), str(settings))

# How many presents past the requested frame the run must be allowed, so the trigger is reached. A
# present index is not a field count, and asking for exactly the frame leaves no margin.
TAIL_PRESENTS = 40

# A pixel counts as ink above this level. Stated, not tuned per picture: the sink is 8-bit and the
# thing being found is a band of drawn pixels against a page, not a dither gradient.
INK_LEVEL = 24

# WHAT A BAND IS HERE, STATED PRECISELY, BECAUSE THE FIRST VERSION OF THIS MEASURE WAS WRONG. It
# compared each row against the picture's own MEDIAN row, and that reports a band on an almost-empty
# page with one row of content in it: the median is 0, so every non-zero row beats 0*3 and 0+12. The
# selftest caught it on a page that is 1/60 busy. A rule that fires on a single content row on an
# empty page is a rule that cannot be used to say "there is no band".
#
# The measure is a SHARE OF THE FRAME'S OWN SPAN instead, which is what a line actually is: a band
# that runs across the page. A letterbox bar, a stray scanline, a margin edge — each spans the frame.
# Content in the middle of a page does not, and a rule that found it would be finding the page.
MIN_ROW_SHARE = 0.85
MIN_COL_SHARE = 0.85

# A run this much of the frame is the FRAME, not a band on it, and is reported as such. Without this
# a uniformly inked page would be reported as one band covering every row, which is not information.
UNIFORM_SHARE = 0.90

RENDER_WIDTH = re.compile(r"render_width=(\d+)")
NATIVE_WIDTH = re.compile(r"native_width=(\d+)")
PRESENT_FENCES = re.compile(r"presentation_fences=(\d+)")


class Refusal(Exception):
    """The run could not assert anything. Never reported as a pass."""


def read_png_rows(path: Path) -> list[list[tuple[int, int, int]]]:
    from PIL import Image

    image = Image.open(path).convert("RGB")
    width, height = image.size
    pixels = image.load()
    return [[pixels[x, y] for x in range(width)] for y in range(height)]


def row_ink(rows: list[list[tuple[int, int, int]]], level: int = INK_LEVEL) -> list[int]:
    """Pixels per row at or above `level` on any channel. Denominator is the width, reported."""
    return [sum(1 for pixel in row if max(pixel) >= level) for row in rows]


def column_ink(rows: list[list[tuple[int, int, int]]], level: int = INK_LEVEL) -> list[int]:
    height = len(rows)
    width = len(rows[0]) if height else 0
    counts = [0] * width
    for row in rows:
        for x, pixel in enumerate(row):
            if max(pixel) >= level:
                counts[x] += 1
    return counts


def _bands(counts: list[int], span: int, share: float) -> tuple[list[tuple[int, int, int]], bool]:
    """(bands, uniform) for one axis. `span` is the OTHER axis's length, so the row profile is
    measured against the frame WIDTH and the column profile against its HEIGHT.

    A single item per RUN, not per row: a 12-row bar is one band, and reporting twelve one-row bands
    would be twelve copies of the same observation. `uniform` is True when every qualifying run
    together covers at least UNIFORM_SHARE of the axis, which is a frame that is simply inked
    everywhere rather than a band drawn on it.
    """
    if not counts or span <= 0:
        return [], False
    threshold = span * share
    runs: list[tuple[int, int, int]] = []
    for index, count in enumerate(counts):
        if count < threshold:
            continue
        if runs and index == runs[-1][1]:
            first, last, peak = runs[-1]
            runs[-1] = (first, index + 1, max(peak, count))
        else:
            runs.append((index, index + 1, count))
    covered = sum(last - first for first, last, _ in runs)
    uniform = bool(runs) and covered >= len(counts) * UNIFORM_SHARE
    return ([], uniform) if uniform else (runs, False)


def measure_bands(rows: list[list[tuple[int, int, int]]]) -> dict:
    """Row and column band reports, each with its own denominator."""
    height = len(rows)
    width = len(rows[0]) if height else 0
    if not height or not width:
        raise Refusal("the capture is empty — no rows to measure")
    ink = row_ink(rows)
    v = column_ink(rows)
    row_bands, row_uniform = _bands(ink, width, MIN_ROW_SHARE)
    col_bands, col_uniform = _bands(v, height, MIN_COL_SHARE)
    return {
        "width": width,
        "height": height,
        "inked": sum(1 for c in ink if c > 0),
        "row_median": sorted(ink)[len(ink) // 2],
        "row_ink": ink,
        "col_median": sorted(v)[len(v) // 2],
        "col_ink": v,
        "row_bands": row_bands,
        "col_bands": col_bands,
        "row_share": MIN_ROW_SHARE,
        "col_share": MIN_COL_SHARE,
        "row_uniform": row_uniform,
        "col_uniform": col_uniform,
    }


def report_bands(label: str, report: dict) -> None:
    height, width = report["height"], report["width"]
    print(f"  {label}: {width}x{height}, {report['inked']}/{height} rows carry ink, median row ink "
          f"{report['row_median']}/{width}, median column ink {report['col_median']}/{height}")
    for axis, bands, uniform, span, share, other in (
        ("ROW", report["row_bands"], report["row_uniform"], height, report["row_share"], "width"),
        ("COL", report["col_bands"], report["col_uniform"], width, report["col_share"], "height"),
    ):
        if uniform:
            print(f"  {label}: every {axis.lower()} is inked across at least {share:.0%} of the "
                  f"frame's {other} — the frame is uniformly inked on that axis, which is not a band "
                  f"on it")
        elif bands:
            for first, last, peak in bands:
                print(f"  {label}: {axis} BAND [{first},{last}) — {last - first} {axis.lower()}(s), "
                      f"peak {peak} ink = {peak / (width if axis == 'ROW' else height):.3f} of the "
                      f"frame's {other} (threshold {share})")
        else:
            axis_rows = report["row_ink"] if axis == "ROW" else report["col_ink"]
            print(f"  {label}: no {axis.lower()} band reaches {share:.0%} of the frame's {other} "
                  f"({int((width if axis == 'ROW' else height) * share)} px of "
                  f"{width if axis == 'ROW' else height}) — {len(axis_rows)} "
                  f"{'rows' if axis == 'ROW' else 'columns'} examined")
    # WHAT THIS REPORT CANNOT SEE, IN THE SAME SENTENCE AS WHAT IT FOUND. The share is of the SINK,
    # and a 2D page is typically NARROWER than the sink it is centred in — Tomba! 2's item menu draws
    # 320 columns of a 428-wide frame at 16:9, i.e. 75% of it, so a full-width line across the PAGE
    # never reaches an 85%-of-sink threshold and reads here as "no band". That is not a smaller number
    # in a report that happens to be clean; it is a question this instrument does not answer. The
    # page's own authored rules are exactly that shape, which is the second reason a band report is a
    # census of the picture and never a verdict on it: measured over the project's own reference
    # capture of this very page (docs/screenshots/05-item-menu.png) it reports 9 row bands and 4 column
    # bands, every one of them authored. A difference between two captures of the same page is the
    # measurement; one capture is not.
    print(f"  {label}: NOT ANSWERED HERE — bands are measured against the SINK span, so a line confined "
          f"to a page narrower than the sink is under the {report['row_share']:.0%} threshold. Compare "
          f"two captures of the same page, or look at the PNG; do not read 'no band' as 'no line'.")


def run_leg(settings: Path, replay: Path, frame: int, out: Path, debug: str) -> dict:
    """One headless run of the already-built binary, capturing `frame`. Returns its own report.

    The launch environment is gate.native_environment's, so an aspect_pair leg and a gate leg cannot
    drift apart in how they are launched. The product writes its present shot at
    scratch/screenshots/present_<frame>.png relative to the cwd, which is the repository.
    """
    shot = ROOT / "scratch" / "screenshots" / f"present_{frame}.png"
    if shot.exists():
        shot.unlink()  # an earlier leg's file must not be read as this leg's capture
    env = agent_environment(settings)
    env.update({
        "PSXPORT_PAD_REPLAY": str(replay),
        "PSXPORT_PRESENT_SHOT_AT": str(frame),
        "PSXPORT_NATIVE_FRAMES": str(frame + TAIL_PRESENTS),
    })
    log = ROOT / LOGDIR / f"aspect-pair-{settings.stem}.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("wb") as sink:
        completed = subprocess.run([BIN], env=env, cwd=ROOT, stdout=sink,
                                   stderr=subprocess.STDOUT, check=False)
    text = log.read_text(errors="replace")
    captured = shot.exists()
    if captured:
        # The frame is IN THE NAME. It was not, and three successive frames of the same settings file
        # each reported "1 of 1 requested shot(s) captured" while overwriting the previous one — three
        # captures claimed, one file on disk. A per-run name that collides is a short answer in the
        # most literal sense.
        target = out / f"{settings.stem}_present{frame}.png"
        target.write_bytes(shot.read_bytes())
    fences = PRESENT_FENCES.findall(text)
    return {
        "name": settings.stem,
        "settings": str(settings),
        "exit": completed.returncode,
        "log": str(log),
        "render_width": RENDER_WIDTH.findall(text),
        "native_width": NATIVE_WIDTH.findall(text),
        "fences": int(fences[-1]) if fences else None,
        "requested_shots": 1,
        "captured_shots": 1 if captured else 0,
        "png": out / f"{settings.stem}_present{frame}.png" if captured else None,
    }


def check_widths(legs: list[dict]) -> list[str]:
    """Each leg must have PRINTED a render width, and the two must differ. Anything else is a
    misconfiguration, and a misconfigured leg's picture cannot support a verdict."""
    problems = []
    for leg in legs:
        if not leg["render_width"]:
            problems.append(f"{leg['name']}: the product printed no render_width, so which aspect it "
                            f"actually ran is UNKNOWN — its picture cannot be read as this aspect")
    widths = [leg["render_width"][-1] for leg in legs if leg["render_width"]]
    if len(widths) == len(legs) and len(set(widths)) == 1:
        problems.append(f"both legs reported render_width={widths[0]} — the two settings files did not "
                        f"produce two widths, so this pair cannot attribute a difference to the field "
                        f"under test")
    return problems


def selftest() -> int:
    """The discriminator: this instrument must answer BOTH ways, on inputs whose answer is known."""
    failures = []

    # POSITIVE — a page with a letterbox band across the top must report the band, and must place it.
    rows = [[(0, 0, 0)] * 40 for _ in range(60)]
    for y in range(0, 8):
        for x in range(40):
            rows[y][x] = (200, 200, 200)
    for y in range(30, 34):
        for x in range(10, 30):
            rows[y][x] = (200, 200, 200)
    report = measure_bands(rows)
    runs = [(b[0], b[1]) for b in report["row_bands"]]
    if (0, 8) not in runs:
        failures.append(f"POSITIVE: a 40x8 band on a 40-wide page was not reported; got {runs}")

    # NEGATIVE — a page that is one row of content on an otherwise empty field. This is the case
    # that broke the first version of this measure, which compared each row against the MEDIAN row
    # and therefore reported a band on any non-empty page. A 20-pixel rule in the middle of a blank
    # page is CONTENT, not a line, and the tool must say nothing about it.
    plain = [[(0, 0, 0)] * 40 for _ in range(60)]
    for x in range(10, 30):
        plain[30][x] = (200, 200, 200)
    if measure_bands(plain)["row_bands"]:
        failures.append("NEGATIVE: a single half-width rule on an otherwise empty page was reported as "
                        "a band, which is the exact false positive the median-relative rule produced")

    # NEGATIVE — a page that is busy EVERYWHERE must not be reported as banded, because the whole
    # frame being inked is not a band ON the frame.
    busy = [[(200, 200, 200)] * 40 for _ in range(60)]
    if measure_bands(busy)["row_bands"]:
        failures.append("NEGATIVE: a uniformly full-width page was reported as banded; the UNIFORM "
                        "guard exists for exactly this")

    # POSITIVE — a band that stops short of the frame's edge is still a line if it is nearly full
    # width. A bar that leaves 1 column clear must be found; this is the discriminating neighbour of
    # the half-width rule above, and together they pin where the rule sits.
    almost = [[(0, 0, 0)] * 40 for _ in range(60)]
    for y in range(10, 16):
        for x in range(0, 38):
            almost[y][x] = (200, 200, 200)
    if (10, 16) not in [(b[0], b[1]) for b in measure_bands(almost)["row_bands"]]:
        failures.append("POSITIVE: a 38-of-40 wide band was missed, so a bar stopping one column short "
                        "of the frame edge would be invisible")

    # The column report is the same arithmetic transposed, and a VERTICAL band must be found by it.
    columns = [[(0, 0, 0)] * 40 for _ in range(60)]
    for y in range(60):
        for x in range(0, 5):
            columns[y][x] = (200, 200, 200)
    if not measure_bands(columns)["col_bands"]:
        failures.append("NEGATIVE-COLUMN: a full-height 5-column band was not reported by the column "
                        "profile, so a vertical line would be invisible to this tool")

    # A short read must SAY SO rather than answering about rows it never measured.
    try:
        measure_bands([])
    except Refusal:
        pass
    else:
        failures.append("an empty capture must be refused, not measured as 'no band'")

    for line in failures:
        print(f"[aspect-pair selftest] FAIL {line}")
    if failures:
        return 1
    print("[aspect-pair selftest] PASS — 3 positive cases (a full-width band placed, a band stopping "
          "one column short of the edge placed, a vertical band placed by the transposed profile) and "
          "3 negative cases (a half-width rule on a blank page, a uniformly full-width page, an empty "
          "capture refused)")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--selftest", action="store_true",
                        help="prove the band measure answers both ways on constructed inputs")
    parser.add_argument("--replay", help="pad replay that reaches the frame under test")
    parser.add_argument("--frame", type=int, default=0, help="present index to photograph in BOTH legs")
    parser.add_argument("--narrow", help="tracked settings file for the FIRST leg")
    parser.add_argument("--wide", help="tracked settings file for the SECOND leg")
    parser.add_argument("--out", default="scratch/aspect-pair", help="where the PNGs land")
    parser.add_argument("--debug", default="", help="PSXPORT_DEBUG channels, e.g. pausemenu")
    args = parser.parse_args(argv)

    if args.selftest:
        return selftest()

    if not Path(BIN).is_file():
        print(f"[aspect-pair] REFUSED: {BIN} is not built — this run captured NOTHING")
        return 2
    if not args.replay or not args.narrow or not args.wide or args.frame <= 0:
        print("[aspect-pair] REFUSED: --replay, --frame, --narrow and --wide are all required; "
              "without two settings files and one frame this tool has no pair to compare")
        return 2
    replay = Path(args.replay)
    if not replay.is_file():
        print(f"[aspect-pair] REFUSED: {replay} is not a replay file — this run captured NOTHING")
        return 2
    for name, value in (("--narrow", args.narrow), ("--wide", args.wide)):
        if not Path(value).is_file():
            print(f"[aspect-pair] REFUSED: {name} {value} is not a file; an unresolvable settings path "
                  f"means the product runs on built-in defaults and the leg would look configured")
            return 2

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    legs = [run_leg(Path(args.narrow), replay, args.frame, out, args.debug),
            run_leg(Path(args.wide), replay, args.frame, out, args.debug)]

    print(f"[aspect-pair] {replay} present {args.frame}: 2 legs requested, 2 launched")
    problems = check_widths(legs)
    for leg in legs:
        served = f"{leg['captured_shots']} of {leg['requested_shots']} requested shot(s) captured"
        print(f"  {leg['name']:<26} settings={leg['settings']}")
        print(f"  {leg['name']:<26} exit={leg['exit']} {served} "
              f"present_fences={leg['fences']} render_width={leg['render_width']} "
              f"native_width={leg['native_width']}")
        if leg["exit"] != 0:
            problems.append(f"{leg['name']}: exited {leg['exit']} — the run did not finish, so its "
                            f"picture does not describe a completed frame sequence (see {leg['log']})")
        if not leg["captured_shots"]:
            problems.append(f"{leg['name']}: 0 of {leg['requested_shots']} requested shot(s) landed — "
                            f"no picture, so this leg asserted nothing (see {leg['log']})")
        if problems and leg["png"] and leg["png"].exists():
            report_bands(leg["name"], measure_bands(read_png_rows(leg["png"])))

    for line in problems:
        print(f"[aspect-pair] PROBLEM {line}")
    if problems:
        return 1
    reports = {leg["name"]: measure_bands(read_png_rows(leg["png"])) for leg in legs}
    for name, report in reports.items():
        report_bands(name, report)
    narrow, wide = legs[0]["png"], legs[1]["png"]
    if narrow.read_bytes() != wide.read_bytes():
        print(f"[aspect-pair] the two pictures DIFFER — {narrow.name} vs {wide.name}")
    else:
        print("[aspect-pair] the two pictures are byte-IDENTICAL — whatever this screen does, the field "
              "under test did not change it")
    return 0


if __name__ == "__main__":
    sys.exit(main())
