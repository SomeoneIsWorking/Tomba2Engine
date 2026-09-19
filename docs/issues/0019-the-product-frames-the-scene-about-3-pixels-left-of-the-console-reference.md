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

The 320x240-vs-320x224 height difference is NOT part of it and is not a defect: the title declares
`GameConfig::guestDisplayHeight = 224` and a native render path deliberately presents more rows than a
console scans out, on a recorded decision (`display_scanout.h`, USER 2026-08-19: "PC is fine, oracle
isn't"). The oracle crops the product to the count it reports and the reference to its own active
area, and the two then agree independently.

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

### That conclusion was OVER-CLAIMED, corrected within the hour

The paragraph that stood here said this proved a display/viewport origin difference and cleared the
camera. Two things are wrong with that.

**A uniform shift rules out camera TRANSLATION, not camera ROTATION.** Parallax is what a camera
moving sideways produces. A camera that YAWS a fraction of a degree shifts the whole image by nearly
the same amount at every depth — which is exactly the uniform signature measured. So the parallax test
narrowed the candidates and did not pick one.

**"The camera agrees in RAM" is not established either.** The declared `camera.mode` and
`camera.angles` agree, but they are the mode byte and three angle halfwords. The camera's composed
look/position state is assembled into the SCRATCHPAD (S+0, S+6, S+8, and the world readout at
0x1F8000D2/D6/DA), and the console reference cannot read scratchpad at all. The follow accumulators
(cam[0x0c/0x14/0x18/0x24/0x28/0x34]) are not in the declared set either. So the camera is unobserved
where it matters, not observed-and-equal.

**And the offset is not frame-constant**, which a fixed viewport origin would be:

```
free_roam   (near-black)  best dx=-1 dy=+0   24.64% -> 23.88%   (improvement 0.76pt: a flat minimum)
played-180f (lit)         best dx=-3 dy=+1   85.78% -> 65.38%   (improvement 20.4pt: decisive)
```

The dark frame has almost no features and its minimum is too flat to carry weight on its own, so this
is suggestive rather than conclusive. But it points the other way from a fixed origin: `played-180f`
is 180 frames of camera-following after free_roam, which is room for a small yaw difference to
accumulate, and drift is what a growing offset would look like.

**The open question is therefore: is the offset constant across the route, or does it grow?** Constant
means viewport/display origin (work it with 0020). Growing means the camera diverges during play,
and the first thing to fix is that the oracle cannot see the camera state that matters.

## Not the whole story

dx=-3 dy=+1 leaves **65.38%** still differing at full resolution, so registration explains part of the
difference and not most of it. The remainder is the shading question in issue 0018; do not treat
fixing this offset as fixing that number.

(The first pass quoted 73.81% for the same offset. That search sampled every 2nd row and column to
stay cheap; the 65.38% above is the full-resolution figure and supersedes it. The offset itself is
identical either way.)

## Next

1. Measure the offset at several points along the route (play 60 / 120 / 240). Constant vs growing
   separates viewport origin from camera drift, and nothing else should be built until it is known.
2. If constant: read the guest's GP1 display-start (0x05) and display-range (0x06/0x07) writes on both
   cores and compare them with where each side places the image. Note the product HLEs the guest's
   display setup and never writes GP1(07) — `external/psxport/runtime/psx/display_scanout.h` is the
   authority on how the presented and scanned counts are resolved.
3. If growing: the declared camera set is the defect to fix first. It admits a frame as comparable
   while the quantity that decides framing is invisible to it. The composed camera lives in scratchpad
   that the reference cannot read, so this needs a main-RAM camera quantity, or an explicit statement
   in the title that camera framing is outside what this oracle can gate.
