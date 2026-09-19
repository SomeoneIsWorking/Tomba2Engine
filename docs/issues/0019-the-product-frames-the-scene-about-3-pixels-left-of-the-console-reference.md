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

## Not the whole story

dx=-3 dy=+1 leaves 73.81% still differing, so registration explains part of the difference and not
most of it. The remainder is the shading question in issue 0018; do not treat fixing this offset as
fixing that number.

## Next

1. Compare the composed camera/view transform between the two cores at this frame — the product can
   read its own scratchpad, so this is a direct read, not an inference from pixels.
2. Check the projection/viewport origin against the reference's active display area. The product
   presents 240 lines and the reference 224 (issue 0020), and a vertical origin difference is a
   plausible shared cause of the dy=+1 half.
