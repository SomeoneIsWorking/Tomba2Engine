---
id: 13
title: HUD weapon carousel missing entirely under pc_render
status: done
labels: [render]
created: 2026-07-22
updated: 2026-07-22
evidence: docs/reference/issues/issue12_13_missing_flame_object.png
---

Same method and same run as the torch-flame card. A dark blue spiky ball with a white squiggle (enemy or prop) sits on the grass under psx_render and is ABSENT under pc_render, at all four sampled scene points - so it is a persistent object, not a one-frame effect. Screen region approx (150,190)-(220,235). Evidence: docs/reference/issues/issue12_13_missing_flame_object.png row 2 (pc | oracle | pc | oracle). Same first suspect as the flame: dropped by the 2D-only OT walk for lack of a depth tag, OR its per-type render handler is unowned so nothing native draws it. Check whether its node reaches Render::fieldObjectsRender at all (that walk now declares the owning node, so 'debug objid' will label anything it does render).

**2026-07-22:** CORRECTION (USER 2026-07-22): the spiky thing is NOT an enemy or a world prop - it is the HUD WEAPON CAROUSEL. So this is a missing HUD element, not missing world geometry, and the first suspect changes: it is a 2D HUD-layer producer, so obj_depth/billboard promotion is the WRONG lead. Check instead why its prims never reach RQ_HUD: the field's 2D-only OT walk keeps genuine 2D-overlay polys (s_ot_2d_drawn path in runtime/recomp/gpu_native.cpp) but a HUD producer whose span is misclassified as world - or dropped by native-cover - would vanish exactly like this. Its screen position is fixed, so it is trivially re-checkable from any camera. Title updated.

**2026-07-22:** FIXED — field_hud.cpp: the 'carousel' is the equipped-WEAPON STRIP (FUN_80025B78: prev/cur/next icons at y=0xD4, DR_AREA clip window x=[0x90,0xAF]) under the FUN_80025D98 field-HUD gate; drawn via generalized Render::emitUiFt4/emitUiSprites (FUN_8007E1B8/8007E6DC full semantics incl. layout modes 0/1/2/3/5); fieldHudRender() called from renderField. Menu emitters now delegate to the general cores (0 px menu regression). Verified: strip crop pixel-identical to oracle (strip_cmp.png), default-leg A/B strip region diff 0 px. Residual: overlay-resident sub-drawers modes 2/7 + task-sm==6 pages unowned. See render.md 2026-07-22 entry.
