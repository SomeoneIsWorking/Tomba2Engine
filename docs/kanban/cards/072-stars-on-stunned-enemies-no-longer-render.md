---
id: 72
title: Stars on stunned enemies no longer render
status: done
labels: [render]
created: 2026-08-04
updated: 2026-08-21
---

**2026-08-04:** USER-REPORTED 2026-08-04 as part of 'last week broke many things'. Stars that appear over a stunned enemy no longer show. Regression window: 2026-07-28..07-31 (the user was away Tue-Fri; that is when the damage landed). UNVERIFIED which commit — do not guess, bisect. Note the framework took heavy render work in that window (widescreen, native depth, GPU present) and psxport 6dda8528/afca817d/28262159 are all in range.

**Resolved 2026-08-21.** This is the same defect as #55 and predates the stated regression window.
The missing prerequisite is now checked in: `replays/bugs/stun-stars.pad` performs a real Circle hit
and produces a live, visible ring node with a non-null owner and the owner's stun bit set. Its texture
state is valid (`rec0=0x8009DDDC`, `tpage=0x15`, `clut=0x7C15`), so the previous debug-spawn owner and
texture theories are falsified.

The root cause is the native transform chain in `Render::fxSpriteRender`. The guest performs
`RotMatrix(node+0x48)` **then** `Math::matColScale` from bytes `0x800A1CD4..D6 << 2`. Native divided
the Q12 matrix by 4096 and omitted the column scale. Those are paired errors: the former collapses
the four stars to one sub-pixel point, while correcting it alone makes the ring 64x too large.
The producer now feeds the Q12 rotation and authored factors through the shared
`MeshQuads::composeScaled` implementation.

True-oracle check: on one replay execution, `renderpath native` at f2799 and live `renderpath psx` at
f2800 both show the four-star cluster around the stunned enemy. The native `fxsprite` line reports
four distinct centres spanning about 28x8 pixels and `drawn=4/4`. `tomba_mesh_quads_math` is the
negative regression case: raw Q12 or the old divide-by-4096 conversion both fail its expected
Q12-plus-column-scale output.
