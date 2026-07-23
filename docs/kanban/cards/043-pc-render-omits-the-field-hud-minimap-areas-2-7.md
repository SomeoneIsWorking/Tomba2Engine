---
id: 43
title: pc_render omits the field HUD minimap (areas 2, 7)
status: done
labels: [render]
created: 2026-07-23
updated: 2026-07-23
---

Found 2026-07-23 by the corrected area sweep. In areas 2 (green machine room) and 7 (jungle-cave), psx_render draws a HUD minimap in the bottom-right corner; pc_render does not. Same game state otherwise (scratch/screenshots/warpsweep/s{2,7}_{pc,psx}.png). This is a field HUD-layer producer gap, sibling to #13 (the equipped-weapon strip, fixed via field_hud.cpp) — the minimap is a DIFFERENT HUD element. Likely the same failure mode: its prims are emitted only into the guest OT that pc_render no longer walks, with no native producer. RE the minimap drawer and own it (tap pattern, RQ_HUD). Not seen on every area because not every area has the minimap up in this capture.

**2026-07-23:** FIXED 2026-07-23. Emitter identified from guest data: debug otattr -> fn=0x80113628 emitting a 0x65 SPRT at (240,144) + its DR_MODE; codemap named the caller as the field HUD dispatcher leaf_80025D98, whose mode-2 / mode-7 branches call the OVERLAY-RESIDENT drawers 0x80113628 / 0x801140A0 (field_hud.cpp already had a comment saying so and bailing). Ghidra on live area-2/area-7 RAM dumps shows the two are the SAME routine with per-area data: a 64x64 RAW map rect + an 8x8 blinking player dot whose position is the player's world pos (scratchpad 0x1F800160/0x1F800164) through the area's own linear map transform (/0xB6 + per-area origin; area 7's map is turned a quarter turn, area 2's Y is negated). Ported as one parameterised producer, game/render/minimap.cpp, RQ_OVERLAY, read-only. Gate: the 64x64 map rect goes 3422 -> 88 differing px (area 2) and 2220 -> 533 (area 7) vs a real PSXPORT_RENDER_PSX=1 reference at the same frame; the residual is the map's TRANSPARENT border, where the pre-existing cave-dither delta shows through (the diff mask is a black hole over the map's own content).
