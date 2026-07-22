---
id: 26
title: Temple interior (area 12): ceiling beam band renders warped/displaced under pc_render
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-22
---

Found 2026-07-22 by the area sweep (REPL `warp 12`). Not a missing layer — a MISPLACED one. The horizontal ceiling/beam band across the top of the room draws as a jagged, arcing thatch under pc_render where psx_render draws a straight horizontal beam. Crops: scratch/screenshots/layergap/x12top_A.png (pc) vs x12top_B.png (psx); full frames x12_A/_B/_D.png (1708 px renderer-attributable, 1423 of them psx-brighter — the largest non-speckle result in the whole 24-area sweep). Motion is excluded by construction: the A/B/C discriminator only counts pixels where the two pc-render neighbour frames AGREE and both differ from the psx frame. Repro: newgame, run 3000, `warp 12`, run 600, shot, `renderpsx`, run 1, shot.
