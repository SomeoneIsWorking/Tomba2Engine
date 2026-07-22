---
id: 42
title: pc_render omits the far BACKGROUND/SKY plane in non-seaside areas (10, 11, 14, 21)
status: todo
labels: [render]
created: 2026-07-23
updated: 2026-07-23
---

Found 2026-07-23 by the CORRECTED area sweep (tools/warpsweep.sh, real PSXPORT_RENDER_PSX=1 reference at the same frame index; recipe newgame; run 3000; warp N; run 600). In areas 10 (lava cave), 11 (snow night), 14 (waterfall) and 21 (wolf-ride, blue sky) the FOREGROUND is byte-for-byte the same state on both legs (character/platforms/props coincident) but psx_render draws the far backdrop plane (cave walls + distant lava / night sky + stars / waterfall / blue sky) while pc_render draws PURE BLACK there. Evidence: scratch/screenshots/warpsweep/s{10,11,14,21}_{pc,psx}.png. >8/255 whole-frame deltas 69731 / 39303 / 68703 / 61375 vs the ~23k generic baseline.

MECHANISM (from docs/findings/render.md, the 2026-06-26 backdrop finding): pc_render's native backdrop producer is SEASIDE-SPECIFIC — ov_bg_tilemap_native publishes the seaside field's backdrop texpage and the classifier routes it to RQ_BACKGROUND. Non-seaside areas have their own backdrop tilemap with no native producer, and since the break-first render rebuild pc_render no longer walks the guest OT, those backdrops are drawn by nobody -> black. The fix is a general native backdrop producer keyed on the active area's backdrop tilemap, not a per-area special case, and NOT a re-introduction of the OT walk or a span tag (banned by NATIVE PRESENTATION). RE the area backdrop drawer and own it.

THIS IS THE CLASS kanban #25 CLAIMED DID NOT EXIST ('missing-layer class NOT distributed across places, 24 of 32 clean'). That claim rested on a pc-vs-pc compare (bare renderpsx no-op) and is falsified — at least 4 areas have a missing backdrop. See docs/findings/render.md 2026-07-23.
