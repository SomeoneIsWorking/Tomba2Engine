#!/usr/bin/env python3
"""ab_motion.py — find effects that are MISSING or FROZEN under pc_render, by comparing MOTION
rather than pixels.

WHY NOT A PIXEL DIFF: pc_render and psx_render are different rasterizers. A whole-frame pc-vs-psx
pixel diff lights up everywhere (colour, dither, filtering) and hides the thing you care about —
that is instrument I005, already distrusted for exactly this. So this tool never compares a pc
pixel to a psx pixel. It compares each leg AGAINST ITSELF over time:

    motion map (leg) = for each tile, did this tile's pixels change across the capture?

then diffs the two MAPS. A tile that MOVES on the reference leg and is STATIC on the pc leg is an
effect that is missing, frozen, or not animating — the exact class of "missing effects" bugs. The
reverse (moves on pc, static on reference) is a spurious pc-only animation. Both are reported.

Because it is self-relative, it survives every colour/AA difference between the legs, and it does
not need the two captures to be frame-aligned — only to cover the same span of gameplay.

CAPTURE (both legs start free-roam at frame 216 under PSXPORT_AUTO_SKIP, so the same driving script
puts them in the same place):

    # reference leg
    PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_NOPACE=1 \
      PSXPORT_DEBUG_SERVER=5971 PSXPORT_AUTO_SKIP=1 ./build/bin/tomba2_port <MAIN.EXE>
    # pc leg: same, minus PSXPORT_GATE / PSXPORT_RENDER_PSX, on another port
    # then shoot N frames into two directories with identical driving, e.g. via dbgclient `shot`

    tools/ab_motion.py scratch/screenshots/ab_ref scratch/screenshots/ab_pc

Any two directories of same-sized PNGs work; files are walked in sorted order.
"""
import argparse, os, sys

try:
    from PIL import Image, ImageChops
except ImportError:
    sys.exit("ab_motion: needs Pillow")


def motion_map(d, tile, thresh):
    """tile -> how many consecutive-frame steps changed it. Frames walked in sorted filename order."""
    files = sorted(f for f in os.listdir(d) if f.lower().endswith((".png", ".ppm")))
    if len(files) < 2:
        sys.exit(f"ab_motion: {d} has {len(files)} frames — need at least 2")
    imgs = [Image.open(os.path.join(d, f)).convert("RGB") for f in files]
    w, h = imgs[0].size
    counts = {}
    for a, b in zip(imgs, imgs[1:]):
        if b.size != (w, h):
            continue
        diff = ImageChops.difference(a, b)
        for y in range(0, h, tile):
            for x in range(0, w, tile):
                box = (x, y, min(x + tile, w), min(y + tile, h))
                bb = diff.crop(box)
                # a tile counts as moving only if some channel moved by more than `thresh`,
                # so dither shimmer / 1-LSB noise does not read as animation
                if max(bb.getextrema(), key=lambda mm: mm[1])[1] > thresh:
                    counts[(x, y)] = counts.get((x, y), 0) + 1
    return counts, (w, h), len(imgs)


def render(grid, w, h, tile, active):
    out = []
    for y in range(0, h, tile):
        out.append("  " + "".join("#" if grid.get((x, y), 0) >= active else "."
                                  for x in range(0, w, tile)))
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ref_dir", help="reference leg frames (psx_render under PSXPORT_GATE=1)")
    ap.add_argument("pc_dir", help="pc_render leg frames")
    ap.add_argument("--tile", type=int, default=16)
    ap.add_argument("--thresh", type=int, default=12, help="per-channel delta that counts as motion")
    ap.add_argument("--active", type=int, default=2, help="min changed steps for a tile to be 'moving'")
    args = ap.parse_args()

    ref, (w, h), nref = motion_map(args.ref_dir, args.tile, args.thresh)
    pc, (w2, h2), npc = motion_map(args.pc_dir, args.tile, args.thresh)
    if (w, h) != (w2, h2):
        sys.exit(f"ab_motion: frame sizes differ ({w}x{h} vs {w2}x{h2})")

    print(f"reference: {nref} frames from {args.ref_dir}")
    print(f"pc       : {npc} frames from {args.pc_dir}")
    print(f"\nMOTION on the REFERENCE leg (tile={args.tile}, thresh={args.thresh}):")
    print(render(ref, w, h, args.tile, args.active))
    print(f"\nMOTION on the PC leg:")
    print(render(pc, w, h, args.tile, args.active))

    missing = sorted(t for t in ref if ref[t] >= args.active and pc.get(t, 0) < args.active)
    extra = sorted(t for t in pc if pc[t] >= args.active and ref.get(t, 0) < args.active)

    print(f"\nMISSING motion — moves on the reference, static under pc_render ({len(missing)} tiles):")
    print(render({t: 1 for t in missing}, w, h, args.tile, 1))
    for t in missing[:20]:
        print(f"    tile ({t[0]:3d},{t[1]:3d})  ref moved {ref[t]}x, pc moved {pc.get(t,0)}x")
    print(f"\nPC-ONLY motion — moves under pc_render, static on the reference ({len(extra)} tiles):")
    for t in extra[:20]:
        print(f"    tile ({t[0]:3d},{t[1]:3d})  pc moved {pc[t]}x, ref moved {ref.get(t,0)}x")
    print("\nA MISSING block is an effect that does not animate (or is not drawn) under pc_render. "
          "Cross-check it with a screenshot pair before filing — the legs must have been driven "
          "over the same span of gameplay for the comparison to mean anything.")


if __name__ == "__main__":
    main()
