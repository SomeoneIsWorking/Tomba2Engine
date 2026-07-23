---
id: 44
title: pc_render omits the central vortex/portal effect in area 15
status: done
labels: [render]
created: 2026-07-23
updated: 2026-07-23
---

Found 2026-07-23 by the corrected area sweep. Area 15 (golden temple-block room): psx_render draws a dark swirling vortex/portal effect in the centre of the room (plus a ceiling difference); pc_render does not. Same foreground geometry state on both legs (scratch/screenshots/warpsweep/s15_{pc,psx}.png, >8/255 = 39948). A short-lived/animated effect layer with no native producer since the break-first rebuild — same class as the torch flame (#12) and score popup (#18). RE its emitter and own it with a tap; do NOT stamp/tag.

**2026-07-23:** FIXED 2026-07-23. Emitter identified from guest data: debug otattr attributed the missing layer to fn=0x80027A4C caller=0x801143C4 on node 0x800ED9E8 (behaviour 0x801140F4) — a type-0x20 custom-render node whose render fn lives in the A0F OVERLAY, a FOURTH emitter family the type-0x20 branch did not know, so the node fell through and nothing drew. RE (Ghidra on a live area-15 dump + ground-truth ov_a0f_gen_801143C4): three layers — a three-glow LENS FLARE along the anchor->screen-centre line while the behaviour state byte is 1, the node's own record list at the world anchor cued toward black, and a rotating SPHERE of particle sprites (radius node+0x4E, latitude rcos(frame*64)>>5 + k*512, longitude step 4096/(rsin th >> 8)) cued toward a blue far colour. Ported as game/render/fx_vortex.cpp: every point projected through projComposeCamera (no gen body, no gte_op, no guest write); the guest PRNG the emitter consumes per particle is read (Rng::SEED_ADDR) and iterated on a HOST copy. Gates: the producer draws 6329 px of which 2195 were previously-differing and now match, and the flare/inner ring/lower glow land on the reference's pixels (frame residual 37299 is area 15's generic native-shading delta, not this layer). fps60 lerp proved with 'debug vortex': 16 distinct evenly-spaced anchor values across 8 logic frames, each present strictly between its neighbours (a non-lerping producer prints each value twice).
