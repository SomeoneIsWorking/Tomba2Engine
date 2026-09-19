#!/usr/bin/env python3
"""fps60_check.py — find things that DON'T interpolate, straight off an fps60dump capture.

WHY: the fps60 directive is "there should technically be no difference between interpolated and
real frames" (docs/fps60-rework.md). Eyeballing a 60fps stream cannot tell you WHICH object is
stale — a rope that never lerps and a rope that lerps correctly look identical in a still, and in
motion the eye only reports "something judders". This walks the real/interp/real triples
`PSXPORT_DEBUG=fps60dump` writes to scratch/framedump/ and answers it per-REGION:

    real(N-1)  ---- interp ---- real(N)
                     ^ where is it actually?

For every 16x16 tile it classifies the interp pixel block as:
  STALE   — identical to real(N-1) while real(N) differs there  (drawn at the previous endpoint)
  AHEAD   — identical to real(N) while real(N-1) differs        (drawn at the next endpoint)
  BETWEEN — differs from both, which is what a lerped prim looks like
  STATIC  — all three agree (nothing moving here; not evidence either way)

STALE ALONE IS NOT A BUG REPORT, and reading it as one cost a session. When the true motion between
the two real frames is under a pixel, the rasterizer at t=0.5 has to land on one side or the other,
and a tile that lands on an endpoint is correct output, not a prim that failed to lerp. So this also
measures, per tile, the best integer translation that aligns real(N-1) onto real(N):

  STALE/AHEAD with a shift of >= 1px   an object that MOVED and was drawn at an endpoint anyway —
                                       the real defect, and the only one worth chasing
  STALE/AHEAD with a shift of 0px      sub-pixel change (dither, shading, a texel-sampling edge)
                                       quantised onto an endpoint — expected

Measured 2026-09-19 on Tomba! 2's hut interior, 120 triples: all 105 STALE and all 110 AHEAD tiles
had a 0px shift, so that scene has no interpolation failure left at this granularity.

USAGE
  PSXPORT_DEBUG=fps60dump ... ./build/bin/tomba2_port ...     # capture (cap 600 files)
  tools/fps60_check.py                                          # walk scratch/framedump/
  tools/fps60_check.py --dir scratch/framedump --tile 16 --top 12
  tools/fps60_check.py --triple f001234                         # one triple, with a tile map

Filenames come from Fps60::dumpPresent: scratch/framedump/f<fence>_<seq>_<real|interp>.png.
Needs Pillow (already used elsewhere in tools/).
"""
import argparse, os, re, sys
from collections import defaultdict

try:
    from PIL import Image, ImageChops, ImageStat
except ImportError:
    sys.exit("fps60_check: needs Pillow (pip install pillow)")

NAME_RE = re.compile(r'^f(\d+)_(\d+)_(real|interp)\.png$')


def load_frames(d):
    """Ordered [(seq, kind, path)] — seq is the dump counter, which is the true present order."""
    out = []
    for fn in os.listdir(d):
        m = NAME_RE.match(fn)
        if m:
            out.append((int(m.group(2)), m.group(3), os.path.join(d, fn)))
    out.sort()
    return out


def triples(frames):
    """Every real -> interp -> real run, in present order."""
    for i in range(len(frames) - 2):
        a, b, c = frames[i], frames[i + 1], frames[i + 2]
        if a[1] == "real" and b[1] == "interp" and c[1] == "real":
            yield a, b, c


def best_shift(pa, pc, tx, ty, tile, radius=2):
    """The integer (dx,dy) that best aligns pa's tile onto pc's, and how far that is.

    A tile that is STALE because its object genuinely moved shows a non-zero shift; a tile that is
    STALE because a sub-pixel change quantised onto an endpoint shows zero. Offsets that would leave
    the image are skipped rather than clamped, so an edge tile cannot be scored against a repeated
    border column."""
    reference = pc.crop((tx, ty, tx + tile, ty + tile))
    best, best_distance = 0, None
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            box = (tx + dx, ty + dy, tx + dx + tile, ty + dy + tile)
            if box[0] < 0 or box[1] < 0 or box[2] > pa.width or box[3] > pa.height:
                continue
            distance = sum(ImageStat.Stat(ImageChops.difference(pa.crop(box), reference)).mean)
            if best_distance is None or distance < best_distance:
                best_distance, best = distance, max(abs(dx), abs(dy))
    return best


def classify(pa, pb, pc, w, h, tile):
    """Per-tile verdict counts + the tile grid. pa/pc real, pb interp."""
    grid = {}
    for ty in range(0, h, tile):
        for tx in range(0, w, tile):
            box = (tx, ty, min(tx + tile, w), min(ty + tile, h))
            ta, tb, tc = pa.crop(box).tobytes(), pb.crop(box).tobytes(), pc.crop(box).tobytes()
            if ta == tc:
                grid[(tx, ty)] = "STATIC" if tb == ta else "BETWEEN"
            elif tb == ta:
                grid[(tx, ty)] = "STALE"
            elif tb == tc:
                grid[(tx, ty)] = "AHEAD"
            else:
                grid[(tx, ty)] = "BETWEEN"
    return grid


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="scratch/framedump")
    ap.add_argument("--tile", type=int, default=16)
    ap.add_argument("--top", type=int, default=12, help="how many worst regions to print")
    ap.add_argument("--triple", help="analyse only the triple starting at this fence/seq prefix")
    args = ap.parse_args()

    if not os.path.isdir(args.dir):
        sys.exit(f"fps60_check: no capture dir {args.dir} — run with PSXPORT_DEBUG=fps60dump first")
    frames = load_frames(args.dir)
    if len(frames) < 3:
        sys.exit(f"fps60_check: only {len(frames)} dumps in {args.dir} — need at least one "
                 f"real/interp/real triple")

    stale_hits = defaultdict(int)   # tile -> how many triples it was STALE in
    ahead_hits = defaultdict(int)
    # STALE/AHEAD split by whether the content actually translated between the two real frames.
    endpoint_shifts = {"STALE": defaultdict(int), "AHEAD": defaultdict(int)}
    moved_hits = defaultdict(int)   # tile -> how many triples anything moved there at all
    n_triples = totals = 0
    counts = defaultdict(int)

    for a, b, c in triples(frames):
        if args.triple and args.triple not in os.path.basename(a[2]):
            continue
        ia, ib, ic = (Image.open(p[2]).convert("RGB") for p in (a, b, c))
        w, h = ia.size
        if ib.size != (w, h) or ic.size != (w, h):
            print(f"  skip {os.path.basename(b[2])}: size mismatch", file=sys.stderr)
            continue
        grid = classify(ia, ib, ic, w, h, args.tile)
        n_triples += 1
        for tile, verdict in grid.items():
            counts[verdict] += 1
            totals += 1
            if verdict != "STATIC":
                moved_hits[tile] += 1
            if verdict == "STALE":
                stale_hits[tile] += 1
            elif verdict == "AHEAD":
                ahead_hits[tile] += 1
            if verdict in endpoint_shifts:
                endpoint_shifts[verdict][best_shift(ia, ic, tile[0], tile[1], args.tile)] += 1
        if args.triple:
            print(f"tile map for {os.path.basename(b[2])} ({w}x{h}, tile={args.tile}):")
            sym = {"STATIC": ".", "BETWEEN": "-", "STALE": "S", "AHEAD": "A"}
            for ty in range(0, h, args.tile):
                print("  " + "".join(sym[grid[(tx, ty)]] for tx in range(0, w, args.tile)))

    if not n_triples:
        sys.exit("fps60_check: no real/interp/real triples found (is fps60 actually on?)")

    print(f"\n{n_triples} triple(s), tile={args.tile}px")
    for k in ("STATIC", "BETWEEN", "STALE", "AHEAD"):
        print(f"  {k:<8} {counts[k]:6d}  ({100.0 * counts[k] / totals:5.1f}%)")

    def report(name, hits, why):
        if not hits:
            print(f"\nno {name} tiles — {why}")
            return
        print(f"\nworst {name} regions (tile x,y -> triples affected / triples where it moved):")
        for tile, n in sorted(hits.items(), key=lambda kv: -kv[1])[:args.top]:
            print(f"  ({tile[0]:4d},{tile[1]:4d})  {n:4d} / {moved_hits[tile]:4d}")

    report("STALE", stale_hits, "everything that moved was interpolated")
    report("AHEAD", ahead_hits, "nothing snapped early")

    # The number that decides whether any of the above is a defect at all.
    print("\nendpoint tiles by how far their content actually translated between the two real "
          "frames:")
    moved_endpoint = 0
    for verdict in ("STALE", "AHEAD"):
        shifts = endpoint_shifts[verdict]
        total = sum(shifts.values())
        if not total:
            print(f"  {verdict:<6} none")
            continue
        parts = " ".join(f"{px}px:{n}" for px, n in sorted(shifts.items()))
        moved = total - shifts.get(0, 0)
        moved_endpoint += moved
        print(f"  {verdict:<6} {total:5d} tile(s): {parts}   -> {moved} that MOVED and still "
              f"landed on an endpoint")
    if moved_endpoint == 0:
        print("  every endpoint tile had a 0px shift: sub-pixel change quantised onto one side, "
              "which is correct output. No interpolation failure at this tile size.")
    else:
        print(f"  {moved_endpoint} tile(s) translated by a whole pixel or more and were still drawn "
              f"at an endpoint. THAT is the defect; the 0px ones are not.")


if __name__ == "__main__":
    main()
