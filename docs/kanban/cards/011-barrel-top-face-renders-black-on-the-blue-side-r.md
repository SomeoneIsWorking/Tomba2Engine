---
id: 11
title: Barrel top face renders BLACK on the blue side (red side correct)
status: doing
labels: [bug, render]
created: 2026-07-22
updated: 2026-07-22
---

**2026-07-22:** USER 2026-07-22 with two screenshots. The water-pump barrel: on the BLUE side its top/rim face renders solid BLACK (wrong); on the RED side the same face renders correctly (red). Both shots are the same object from different angles, so this is face/normal or CLUT dependent rather than a missing texture - a black face next to a correct one usually means the primitive is drawn with no texture bound (or a zero CLUT), or is back-face-shaded. Leads: Panel::fillQuad is NOT it (2D UI). Start from the per-object emitters just ported - Render::subPartWalk (LIVE, 139 hits) submits per-sub-part geomblks, and the barrel is a multi-part node. Compare the two angles under SBS (pane A pc_faithful vs pane B oracle) to see whether psx_render shows the same black face; if the oracle is correct and ours is black, it is ours.

**2026-07-22:** 2026-07-22 ROOT-CAUSED (not yet fixed). Headless repro found: same exec (PSXPORT_GATE=1) with pc_render vs PSXPORT_ORACLE=1 psx_render, seesaw-weight.pad + walk left/up - pc_render draws the barrel top BLACK, psx_render draws the blue water (scratch/screenshots/cmp/barrelcmp.png). PRIMAT at (165,92): exactly two world prims, the SAME four screen vertices - a dark cap (seq=726, tp 704,0) and the water surface (seq=746, tp 640,0). The guest paints cap-then-water; PSX has no depth buffer so the water wins. We give both real per-vertex depth and the buffer picks the cap, because the cap is genuinely ~3e-5 nearer AND the non-planar quad is triangulated along different diagonals in the two submissions (gap inflated to 5.8e-4, sign flips with angle = black one side, correct the other). PROVED the resolver: PSXPORT_ZBIAS=3e-5 PSXPORT_ZBIAS_MAX=0.5 renders the water, matching the oracle (proof.png) - and PROVED the global knob cannot ship, it wrecks the rest of the frame (full3.png). Fix direction: one shared depth per object so the pair is an exact tie; blocker is dbg_node=0 on OT-walk-teed prims. Full writeup in docs/findings/render.md.
