#!/usr/bin/env python3
"""Post-filter a PSXPORT_PRIMAT `[primat-rq]` log: keep ONLY prims that GENUINELY cover the probe pixel.

WHY THIS EXISTS — the raw instrument reports false positives, measured 2026-08-06 (kanban #77).
`RenderQueue::emitOrQueue`'s in-band point-in-triangle test is

    (w0>=0 && w1>=0 && w2>=0) || (w0<=0 && w1<=0 && w2<=0)

which is TRUE for every pixel on screen when the triangle is DEGENERATE (two or more coincident
vertices): all three edge functions are identically 0. Degenerate prims are common on this path —
a GT3 is teed as a quad with v3==v2, and off-screen/clamped geometry collapses to a point — so a
raw `PSXPORT_PRIMAT` log over any busy frame carries a tail of prims that "cover" a pixel they are
nowhere near. That is exactly how kanban #77 came to record `dbgnode=FFFF0002` (the scene table) as
the producer of area 14's water wall: every one of those hits was degenerate, and the real producer
was `Render::fxBackdropPlaneRender` (node 800ED960), which the raw log also listed.

WHAT IT PRINTS — the denominators, always. Total lines seen, how many were on another frame, how
many were rejected as degenerate/non-covering, and how many genuinely cover. A "0 genuine covers"
answer is only meaningful next to those counts, and the blind spots are printed with it.

USAGE
    PSXPORT_PRIMAT="160,40,3935" ... ./scratch/bin/tomba2_port > run.log
    python3 tools/primat_filter.py run.log 160 40 3940        # x y FRAME-of-the-shot

The third PRIMAT field is a START frame (the 6000-line cap is otherwise spent in the first few
hundred frames); the argument here is the EXACT frame you captured, so pass the frame the shot was
taken on — the port prints it on any per-frame diagnostic (e.g. `[gteflag] f3940 ...`).

SELF-TEST
    python3 tools/primat_filter.py --selftest
Feeds one degenerate prim that MUST be rejected and one real prim that MUST be accepted, so the
filter cannot silently degrade into "rejects everything" (which would read as a clean answer).
"""
import re
import sys
import collections

LINE = re.compile(
    r"f(\d+) seq=(\d+) dbgnode=([0-9A-Fa-f]+).*?nv=(\d+).*?"
    r"xy=\[\((-?\d+),(-?\d+)\) \((-?\d+),(-?\d+)\) \((-?\d+),(-?\d+)\) \((-?\d+),(-?\d+)\)\]")


def _edge(ax, ay, x0, y0, x1, y1):
    return (x1 - x0) * (ay - y0) - (y1 - y0) * (ax - x0)


def covers(px, py, xy, nv):
    """True iff (px,py) is inside a NON-DEGENERATE triangle of this prim."""
    tris = [(0, 1, 2)] + ([(1, 2, 3)] if nv == 4 else [])
    for i0, i1, i2 in tris:
        (x0, y0), (x1, y1), (x2, y2) = xy[i0], xy[i1], xy[i2]
        if (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0) == 0:
            continue                       # degenerate: covers nothing, whatever the edge functions say
        w0 = _edge(px, py, x1, y1, x2, y2)
        w1 = _edge(px, py, x2, y2, x0, y0)
        w2 = _edge(px, py, x0, y0, x1, y1)
        if (w0 >= 0 and w1 >= 0 and w2 >= 0) or (w0 <= 0 and w1 <= 0 and w2 <= 0):
            return True
    return False


def selftest():
    degenerate = [(100, 100), (50, 50), (50, 50), (50, 50)]
    real = [(0, 0), (200, 0), (0, 200), (200, 200)]
    ok = True
    if covers(300, 300, degenerate, 4):
        print("SELFTEST FAIL: a degenerate prim was reported as covering (300,300)"); ok = False
    if not covers(50, 50, real, 4):
        print("SELFTEST FAIL: a real quad was NOT reported as covering its own interior"); ok = False
    print("selftest: PASS (degenerate rejected, real accepted)" if ok else "selftest: FAIL")
    return 0 if ok else 1


def main():
    if "--selftest" in sys.argv:
        return selftest()
    if len(sys.argv) != 5:
        print(__doc__)
        return 2
    path, px, py, frame = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
    tot = wrongframe = rejected = genuine = 0
    hist_rej, hist_hit, rows = collections.Counter(), collections.Counter(), []
    for line in open(path):
        if "primat-rq" not in line:
            continue
        m = LINE.search(line)
        if not m:
            continue
        tot += 1
        if int(m.group(1)) != frame:
            wrongframe += 1
            continue
        node, nv = m.group(3), int(m.group(4))
        xy = [(int(m.group(5 + 2 * k)), int(m.group(6 + 2 * k))) for k in range(4)]
        if covers(px, py, xy, nv):
            genuine += 1; hist_hit[node] += 1; rows.append(line.rstrip())
        else:
            rejected += 1; hist_rej[node] += 1
    print(f"probe=({px},{py}) frame={frame}  file={path}")
    print(f"  [primat-rq] lines total          : {tot}")
    print(f"  rejected: other frame            : {wrongframe}")
    print(f"  rejected: degenerate / no cover  : {rejected}   dbgnodes={dict(hist_rej)}")
    print(f"  GENUINE covering prims           : {genuine}   dbgnodes={dict(hist_hit)}")
    if tot == 0:
        print("  REFUSING TO ANSWER: the log contains NO [primat-rq] lines at all — PSXPORT_PRIMAT")
        print("  was unset, its start frame was never reached, or this is the wrong log. Not a negative.")
        return 1
    if genuine == 0:
        print("  BLIND TO (a hit here would NOT be found): prims emitted outside emitOrQueue (2D/UI")
        print("  direct paths and the gp0 OT walk, which logs as plain [primat] not [primat-rq]);")
        print("  prims on any frame other than the one requested; prims past the 6000-line cap.")
    for r in rows[:12]:
        print("   ", r)
    return 0


if __name__ == "__main__":
    sys.exit(main())
