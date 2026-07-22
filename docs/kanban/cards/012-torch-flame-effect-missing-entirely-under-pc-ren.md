---
id: 12
title: Torch flame effect missing entirely under pc_render
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-22
evidence: docs/reference/issues/issue12_13_missing_flame_object.png
---

Found by pc_render vs psx_render at IDENTICAL exec state (PSXPORT_GATE=1 vs PSXPORT_ORACLE=1, so only the renderer differs). The seaside hut carries a lit TORCH FLAME, clearly present under psx_render and ABSENT under pc_render - not mis-coloured or mis-ordered, simply not drawn. Screen region approx (255,0)-(320,60) at replay point 'newgame; run 10000' on replays/bugs/seesaw-weight.pad. Evidence: docs/reference/issues/issue12_13_missing_flame_object.png (row 1: pc | oracle | pc | oracle) and issue12_13_pc_vs_oracle.png (4 scene points, 3rd column is the abs-diff). Likely mechanism to check FIRST: the field's 2D-only OT walk (s_ot_2d_only in runtime/recomp/gpu_native.cpp) DROPS guest prims that carry no depth tag; a flame is a 2D billboard that must be obj_depth-promoted to survive (see the existing #39 weapon-chain/impact-effect precedent and withDepthTag in game/render/render_internal.h). If the flame's emitter is not wrapped in a depth-tag scope its prims are discarded.
