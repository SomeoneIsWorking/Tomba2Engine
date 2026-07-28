---
id: C012
kind: claim
status: holds
created: 2026-07-28
tags: render
---

## Claim

Eight type-0x20 render fns have no producer and no replay in the library reaches any of them; they live in areas 3, 4, 8, 10, 11, 13, 14 and 21.

## Evidence

22-area warp sweep 2026-07-28 with PSXPORT_DEBUG=nofx: 0x801113B4 (a3, ported: Render::fxMotionTrailRender), 0x8013B118 (a4), 0x80116904 (a8), 0x80113768 (a10, ported: Render::fxCuedSpriteRender), 0x80110C14 (a13, ported: Render::fxRingSpriteRender — see C013), 0x801110BC (a11), 0x80110C14 (a13), 0x80110CA4 (a14), 0x8010C7F4 (a21, ported: Render::fxParticleFieldRender) + 0x8010C1D8 (a21). Every area exits 0, no abort/fatal/miss. Only terrain (0x8002AB5C) and the widescreen margin (0x8013CDD4) appear in all 22, both owned by another route.

## What would falsify it

the sweep samples one instant 600 frames after warp, so a trigger-gated effect can be missing from this list; a longer or driven per-area capture that names more fns does not contradict it, but one that shows any of these eight NOT firing would

UPDATE 2026-07-29: four of the eight now have native producers. NOTE for anyone re-running the sweep: a
single-instant shot 600 frames after warp reported '0 pixels changed' for BOTH of them even though
the ring producer emits 21/21 items every frame — at that instant it is off the bottom of the frame.
See instrument I022 before reading a 0-px result as 'the producer does nothing'.
