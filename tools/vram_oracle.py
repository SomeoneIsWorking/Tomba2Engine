#!/usr/bin/env python3
"""Compare OUR VRAM against the beetle GPU oracle's at one frame of one replay.

WHY THIS EXISTS. "The picture is wrong" is three different claims that need three different
measurements, and only VRAM separates them:

  * the guest issued the wrong commands        -> ours AND beetle both hold the wrong picture
  * our rasterizer is wrong                    -> ours and beetle DIFFER
  * rasterization is fine, presentation is not -> ours == beetle, both hold the RIGHT picture,
                                                  and the screen is still black

Measuring the presented frame cannot tell the third case from "nothing was drawn" -- which is
exactly the mistake this tool exists to stop repeating (kanban #111).

WHAT IT REFUSES TO DO. It does not report a comparison it could not make. A run whose feed the
oracle reports as INCOMPLETE, or which never reached the requested frame, exits 2 and says so,
because a difference measured on a lossy feed is not evidence about either rasterizer.
"""

import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

import numpy as np  # a pure-python 524,288-pixel diff took ~5 min/frame; this is the whole sweep

ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "build/bin/tomba2_port"
SHOTS = ROOT / "scratch/screenshots"


def read_ppm(path):
    with open(path, "rb") as f:
        assert f.readline().strip() == b"P6", f"{path}: not a P6 ppm"
        w, h = map(int, f.readline().split())
        f.readline()
        return w, h, np.frombuffer(f.read(), dtype=np.uint8).reshape(h, w, 3)


def rect_stats(img, x0, y0, rw, rh):
    r = img[y0 : y0 + rh, x0 : x0 + rw]
    tot = r.shape[0] * r.shape[1]
    if tot == 0:
        return None
    return {
        "nz": int((r != 0).any(axis=2).sum()),
        "tot": tot,
        "mean": (
            float(r[..., 0].mean()),
            float(r[..., 1].mean()),
            float(r[..., 2].mean()),
        ),
    }


def diff_count(a, b):
    return int((a != b).any(axis=2).sum())


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("replay", help="path to a .pad recording, repo-relative")
    ap.add_argument("frame", type=int, help="frame to dump and compare")
    ap.add_argument(
        "--path",
        default="psx",
        choices=["psx", "gte", "native"],
        help="PSXPORT_RENDER_PATH for the run (default: psx)",
    )
    ap.add_argument("--timeout", type=int, default=900)
    ap.add_argument(
        "--selftest",
        action="store_true",
        help="inject beetle's 1px positive control; a NON-zero difference is the PASS",
    )
    ap.add_argument(
        "--warp",
        type=int,
        choices=range(22),
        metavar="AREA",
        help="drive newgame, settle, then REPL-warp to AREA before comparing",
    )
    ap.add_argument(
        "--settle",
        type=int,
        default=3000,
        help="frames to run before --warp (default: 3000)",
    )
    ap.add_argument(
        "--dwell",
        type=int,
        default=600,
        help="frames to run after --warp (default: 600)",
    )
    ap.add_argument("--keep-log", default=None)
    args = ap.parse_args()

    if not BIN.exists():
        print(
            f"REFUSED: {BIN} does not exist — build tomba2_port first", file=sys.stderr
        )
        return 2
    if not (ROOT / args.replay).exists():
        print(f"REFUSED: replay {args.replay} does not exist", file=sys.stderr)
        return 2

    # Stale-artefact guard: a previous run's .ppm at this frame reads exactly like this run's output.
    for name in ("ours", "beetle"):
        p = SHOTS / f"{name}_vram_f{args.frame}.ppm"
        if p.exists():
            p.unlink()

    env = dict(os.environ)
    env.update(
        {
            "PSXPORT_VK_HEADLESS": "1",
            "PSXPORT_NOAUDIO": "1",
            "PSXPORT_NO_FMV": "1",
            "PSXPORT_NOPACE": "1",
            "PSXPORT_ASSET_DIR": "external/psxport",
            "PSXPORT_RENDER_PATH": args.path,
            "PSXPORT_GPU_BEETLE": "1",
            "PSXPORT_GPU_BEETLE_DUMP": str(args.frame),
            "PSXPORT_PAD_REPLAY": args.replay,
            "PSXPORT_DEBUG": "gpubeetle,gpu",
            # BOUND THE RUN. The headless frame cap is LIFTED for pad replays (psxport 088c4722), so
            # without this the game runs forever after the recording ends — measured: a title-screen
            # measurement was at frame 134,892 while waiting for frame 1,027. The tail absorbs the
            # replay's own overrun past the frame of interest.
            "PSXPORT_NATIVE_FRAMES": str(args.frame + 40),
        }
    )
    repl_script = None
    if args.warp is not None:
        env["PSXPORT_REPL"] = "1"
        repl_script = (
            f"newgame\nrun {args.settle}\nwarp {args.warp}\nrun {args.dwell}\nquit\n"
        ).encode()
    if args.selftest:
        env["PSXPORT_GPU_BEETLE_SELFTEST"] = "1"

    # STREAM AND FILTER. The oracle emits a census line for EVERY frame, so capturing the whole run
    # produced a 161 MB log per measurement — slow to write, and pure noise around the four lines that
    # answer the question. Keep the lines about the requested frame, plus a rolling tail so a run that
    # died still shows how it died.
    want = (f"f{args.frame} ", f"frame {args.frame}:")
    kept, tail = [], []
    t0 = time.time()
    proc = subprocess.Popen(
        [str(BIN)],
        cwd=ROOT,
        env=env,
        stdin=subprocess.PIPE if repl_script else None,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if repl_script:
        proc.stdin.write(repl_script)
        proc.stdin.close()
    try:
        for raw in proc.stdout:
            line = raw.decode("utf-8", "replace")
            if any(w in line for w in want):
                kept.append(line)
            tail.append(line)
            if len(tail) > 400:
                del tail[:200]
            if time.time() - t0 > args.timeout:
                proc.kill()
                print(
                    f"REFUSED: the run exceeded --timeout {args.timeout}s",
                    file=sys.stderr,
                )
                break
        proc.wait(timeout=30)
    finally:
        if proc.poll() is None:
            proc.kill()
    log = "".join(kept) + "\n---- last lines of the run ----\n" + "".join(tail)
    logpath = (
        Path(args.keep_log)
        if args.keep_log
        else (
            ROOT
            / f"scratch/logs/vram_oracle_{Path(args.replay).stem}_f{args.frame}_{args.path}.log"
        )
    )
    logpath.parent.mkdir(parents=True, exist_ok=True)
    logpath.write_text(
        log
    )  # ALWAYS — a census you cannot re-read means re-running the whole game

    print(
        f"# {args.replay} f{args.frame} path={args.path}"
        f"{' SELFTEST' if args.selftest else ''}  ({time.time() - t0:.0f}s)"
    )

    feed = re.search(rf"f{args.frame} FEED: (.*)", log)
    verdict = re.search(rf"f{args.frame} FEED (COMPLETE|INCOMPLETE): (.*)", log)
    disp = re.search(
        rf"frame {args.frame}: .*disp (\d+)x(\d+) @ \((-?\d+),(-?\d+)\)", log
    )
    if not feed:
        print(
            f"REFUSED: the run never reported a feed census at f{args.frame} "
            f"(reached frame {last_frame(log)}). Nothing was compared.",
            file=sys.stderr,
        )
        return 2
    print(f"  feed: {feed.group(1)}")
    if verdict:
        print(f"  {verdict.group(1)}: {verdict.group(2)}")
    if verdict and verdict.group(1) == "INCOMPLETE" and not args.selftest:
        print(
            "REFUSED: the oracle's feed was lossy at this frame, so a VRAM difference is not "
            "evidence about either rasterizer.",
            file=sys.stderr,
        )
        return 2

    ours_p, beetle_p = (
        SHOTS / f"ours_vram_f{args.frame}.ppm",
        SHOTS / f"beetle_vram_f{args.frame}.ppm",
    )
    if not ours_p.exists() or not beetle_p.exists():
        print(
            "REFUSED: the VRAM dumps were not written; nothing to compare.",
            file=sys.stderr,
        )
        return 2

    w, h, ours = read_ppm(ours_p)
    _, _, beetle = read_ppm(beetle_p)
    d = diff_count(ours, beetle)
    total = w * h
    print(f"  whole VRAM {w}x{h}: differing {d}/{total} ({100 * d / total:.2f}%)")

    if disp:
        dw, dh, dx, dy = (int(g) for g in disp.groups())
    else:
        dw, dh, dx, dy = 320, 240, 0, 0
        print("  NOTE: no display-rect line in the log; assuming 320x240 @ (0,0)")
    print(f"  display rect {dw}x{dh} @ ({dx},{dy}) — what the guest says is on screen:")
    for name, buf in (("ours", ours), ("beetle", beetle)):
        s = rect_stats(buf, dx, dy, dw, dh)
        print(
            f"    {name:7s} non-black {s['nz']}/{s['tot']} ({100 * s['nz'] / s['tot']:.1f}%)"
            f"   mean RGB ({s['mean'][0]:.2f}, {s['mean'][1]:.2f}, {s['mean'][2]:.2f})"
        )

    if args.selftest:
        ok = d > 0
        print(
            f"  SELFTEST: {'PASS' if ok else 'FAIL'} — a known 1px rasterizer difference was "
            f"injected, so a NON-zero VRAM difference is the pass."
        )
        return 0 if ok else 1
    return 0


def last_frame(log):
    m = re.findall(r"frame (\d+):", log)
    return m[-1] if m else "?"


if __name__ == "__main__":
    sys.exit(main())
