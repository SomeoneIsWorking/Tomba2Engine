---
id: 26
title: Area 12 interior: ceiling beam band renders warped/displaced under pc_render
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-23
---

Found 2026-07-22 by the area sweep (REPL `warp 12`). Not a missing layer — a MISPLACED one. The horizontal ceiling/beam band across the top of the room draws as a jagged, arcing thatch under pc_render where psx_render draws a straight horizontal beam. Crops: scratch/screenshots/layergap/x12top_A.png (pc) vs x12top_B.png (psx); full frames x12_A/_B/_D.png (1708 px renderer-attributable, 1423 of them psx-brighter — the largest non-speckle result in the whole 24-area sweep). Motion is excluded by construction: the A/B/C discriminator only counts pixels where the two pc-render neighbour frames AGREE and both differ from the psx frame. Repro: newgame, run 3000, `warp 12`, run 600, shot, `renderpsx`, run 1, shot.

**2026-07-23:** 2026-07-23 TITLE CORRECTED. This card said 'Temple interior' and nothing ever sourced that name — the area sweep agent invented it, and I repeated it. Looking at the card's own evidence (scratch/screenshots/layergap/x12_A.png): the room has papered walls, framed pictures, curtained windows, wooden floor platforms and what reads as furniture. It is a HOUSE or mansion interior, not a temple. The area is simply 12; do not attach a place name to it until one comes from the game (an area/scene name in guest data, or the USER). Same class of error as the '#24 area 22' card, which named an area that does not exist at all — the game has 22 areas, 0..21. The DEFECT on this card is real and unaffected: psx_render draws a straight horizontal timbered ceiling band across the full width; pc_render draws it arcing/sagging with the ends bent down (x12top_B.png vs x12top_A.png), 1708 renderer-attributable px, the largest non-speckle result of the 24-area sweep.
