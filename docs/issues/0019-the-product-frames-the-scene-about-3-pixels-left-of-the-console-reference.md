---
id: 19
title: The product frames the scene about 3 pixels left of the console reference
status: open
symptom: at a settled free-roam frame where every declared guest range agrees byte for byte, shifting the product by dx=-3 dy=+1 drops the picture difference from 86.98% to 73.81%. The camera ranges agree in RAM, so the offset is introduced downstream of the state
state_items: S004
tags: render,camera,oracle,picture
created: 2026-09-19
---

## Measured

`tools/picture_oracle.py --bios ../SCPH1001.BIN --play 180`, on the settled lit scene 180 game frames
past free_roam. The state guard did not refuse: `camera.mode`, `camera.angles`, `player.position`,
`player.motion`, `state_machine`, `area_index` and all three pad words agree byte for byte.

A 2D offset search over dx,dy in [-6,+6] (every 2nd row and column, 17,696 sampled pixels):

```
dx=-3 dy=+1   73.81%     <- minimum
dx=-4 dy=+1   78.17%
dx=-2 dy=+1   79.45%
dx=-3 dy=+0   82.88%
dx=+0 dy=+0   86.98%     <- as compared
```

A clean single minimum with monotone falloff either side. The product's picture sits about 3 pixels
right of where the reference puts it (shifting the product left by 3 improves the match), and one
pixel down.

## Why this is its own defect

The camera state in guest RAM is IDENTICAL on both cores at this frame. So the offset is not the game
deciding to look somewhere else — it is introduced after the state, where the native renderer turns
camera state into a projection and a viewport. That is a first-party, title-owned boundary.

Note the camera's world position is assembled into the scratchpad (0x1F8000D2/D6/DA) and the console
reference cannot read scratchpad, so "the camera ranges agree" means the mode and the three angles
agree — not the composed position. Ruling the composed position in or out is part of this work.

## CORRECTED the same day: it is the VIEWPORT ORIGIN, not the camera

The section above reasoned that because the camera state agrees in RAM, the offset must enter where
the native renderer turns camera state into a projection. Half right, and the half that was wrong was
about to send the work to the wrong place. Repeating the offset search independently in seven regions
of the same frame, at full resolution:

```
region                best dx,dy   residual
whole frame           dx=-3 dy=+1    65.38%
top band  y  0- 74    dx=-3 dy=+1    55.34%
mid band  y 75-149    dx=-3 dy=+1    70.62%
low band  y150-223    dx=-3 dy=+1    70.33%
left   x  0-106       dx=-3 dy=+1    66.22%
centre x107-212       dx=-3 dy=+1    64.01%
right  x213-319       dx=-3 dy=+1    65.92%
```

**Every region agrees exactly.** The top band is distant sky and hillside; the low band is grass at
the player's feet. A camera position or rotation difference moves near content further than far
content — that is parallax, and it is the whole reason the two bands were measured separately. There
is none. The offset is a rigid translation of the finished image.

So this is a DISPLAY/VIEWPORT ORIGIN difference, and comparing composed camera transforms would have
been time spent on a boundary the measurement had already cleared. Issue 0020 (the product presents
240 lines, the reference 224) is now the likely shared cause rather than a footnote, and the two
should be worked together.

## Not the whole story

dx=-3 dy=+1 leaves **65.38%** still differing at full resolution, so registration explains part of the
difference and not most of it. The remainder is the shading question in issue 0018; do not treat
fixing this offset as fixing that number.

(The first pass quoted 73.81% for the same offset. That search sampled every 2nd row and column to
stay cheap; the 65.38% above is the full-resolution figure and supersedes it. The offset itself is
identical either way.)

## Next

1. Work this with issue 0020. Read the guest's GP1 display-start (0x05) and display-range (0x06/0x07)
   writes on both cores at a settled frame and compare them with where each side actually places the
   image. A rigid 3-left/1-down translation should fall straight out of those registers.
2. Do NOT start by comparing composed camera transforms. The per-region result above rules the camera
   out as the cause of this offset.
