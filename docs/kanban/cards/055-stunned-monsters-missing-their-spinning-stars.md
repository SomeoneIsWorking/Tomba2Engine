---
id: 55
title: Stunned monsters missing their spinning stars
status: done
labels: [render, bug]
created: 2026-07-23
updated: 2026-08-21
evidence: docs/reference/issues/issue55_stunned_stars_missing.png
---

USER 2026-07-23 with a side-by-side capture: a STUNNED monster should have SPINNING STARS orbiting its head. The reference side clearly shows the yellow star sprites above/around the stunned enemy; under pc_render they are absent entirely.

**Resolved 2026-08-21; duplicate #72 carries the same evidence.** A new real-input replay,
`replays/bugs/stun-stars.pad`, hits an enemy with Circle and proves the actual game path: node
`0x800EED90` is visible, stays alive, owns render function `0x8002B3A4`, and its real owner has the
stun bit `owner[0x1B] & 0x40`. This falsifies the old debug-spawn theory that `node+0x14` was missing.

The producer already existed, but its transform omitted one guest operation and mis-scaled another.
`FUN_8002B3A4` does `RotMatrix(node+0x48)` and then `Math::matColScale` with three authored bytes at
`0x800A1CD4`, each shifted left by two. Native code divided the Q12 rotation by 4096 and skipped the
column scale. The result was four centres collapsed within one pixel; changing only the first error
made the 6400-unit local ring 64x too large and mostly off-screen. `Render::fxSpriteRender` now uses
the existing `MeshQuads::composeScaled` implementation for the complete chain.

Verification on the replay at f2799/f2800: the native path displays the same four-star cluster as the
live `renderpath psx` software oracle. The native diagnostic reports four distinct centres spanning
about 28x8 pixels, `drawn=4/4`, valid record `0x8009DDDC`, and valid `tpage=0x15/clut=0x7C15`.
`tomba_mesh_quads_math` locks the Q12-plus-column-scale regression.
