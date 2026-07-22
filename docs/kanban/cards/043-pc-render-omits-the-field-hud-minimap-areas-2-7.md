---
id: 43
title: pc_render omits the field HUD minimap (areas 2, 7)
status: todo
labels: [render]
created: 2026-07-23
updated: 2026-07-23
---

Found 2026-07-23 by the corrected area sweep. In areas 2 (green machine room) and 7 (jungle-cave), psx_render draws a HUD minimap in the bottom-right corner; pc_render does not. Same game state otherwise (scratch/screenshots/warpsweep/s{2,7}_{pc,psx}.png). This is a field HUD-layer producer gap, sibling to #13 (the equipped-weapon strip, fixed via field_hud.cpp) — the minimap is a DIFFERENT HUD element. Likely the same failure mode: its prims are emitted only into the guest OT that pc_render no longer walks, with no native producer. RE the minimap drawer and own it (tap pattern, RQ_HUD). Not seen on every area because not every area has the minimap up in this capture.
