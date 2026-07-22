---
id: 11
title: Barrel top face renders BLACK on the blue side (red side correct)
status: todo
labels: [bug, render]
created: 2026-07-22
updated: 2026-07-22
---

**2026-07-22:** USER 2026-07-22 with two screenshots. The water-pump barrel: on the BLUE side its top/rim face renders solid BLACK (wrong); on the RED side the same face renders correctly (red). Both shots are the same object from different angles, so this is face/normal or CLUT dependent rather than a missing texture - a black face next to a correct one usually means the primitive is drawn with no texture bound (or a zero CLUT), or is back-face-shaded. Leads: Panel::fillQuad is NOT it (2D UI). Start from the per-object emitters just ported - Render::subPartWalk (LIVE, 139 hits) submits per-sub-part geomblks, and the barrel is a multi-part node. Compare the two angles under SBS (pane A pc_faithful vs pane B oracle) to see whether psx_render shows the same black face; if the oracle is correct and ours is black, it is ours.
