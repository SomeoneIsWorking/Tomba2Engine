---
id: 12
title: Torch flame effect missing entirely under pc_render
status: done
labels: [render]
created: 2026-07-22
updated: 2026-07-23
evidence: docs/reference/issues/issue12_13_missing_flame_object.png
---

Found by pc_render vs psx_render at IDENTICAL exec state (PSXPORT_GATE=1 vs PSXPORT_ORACLE=1, so only the renderer differs). The seaside hut carries a lit TORCH FLAME, clearly present under psx_render and ABSENT under pc_render - not mis-coloured or mis-ordered, simply not drawn. Screen region approx (255,0)-(320,60) at replay point 'newgame; run 10000' on replays/bugs/seesaw-weight.pad. Evidence: docs/reference/issues/issue12_13_missing_flame_object.png (row 1: pc | oracle | pc | oracle) and issue12_13_pc_vs_oracle.png (4 scene points, 3rd column is the abs-diff). Likely mechanism to check FIRST: the field's 2D-only OT walk (s_ot_2d_only in runtime/recomp/gpu_native.cpp) DROPS guest prims that carry no depth tag; a flame is a 2D billboard that must be obj_depth-promoted to survive (see the existing #39 weapon-chain/impact-effect precedent and withDepthTag in game/render/render_internal.h). If the flame's emitter is not wrapped in a depth-tag scope its prims are discarded.

**2026-07-22:** FIXED — fx_sprite.cpp: FUN_80027A4C leaf tap (gen body + host-side quad re-derivation, flat obj depth from SZ3). Covers all 27A4C callers (flame family 27CB4/27E5C/281EC + #28 grab/dust quads). Verified default-leg same-process renderpsx A/B at f10000 (scratch/screenshots/gaps/ab2_pc.png vs ab2_psx.png + flame_ab2.png — flame present both). NOTE: taps do not fire under PSXPORT_GATE=1 (psx_fallback leg routes to gen), so GATE+pc_render still lacks tap layers — pre-existing class; verify tap fixes with the renderpsx toggle on the default leg. SBS smoke 0-diff f4530. See render.md 2026-07-22 entry.

**2026-07-23:** 2026-07-23: superseded the leaf tap with the native producer (see #23). The GATE=1 limitation noted here ('taps do not fire under PSXPORT_GATE=1') is now GONE for this family — the native producer runs in the pc_render walk on every exec leg, so the flames render under GATE=1 too.
