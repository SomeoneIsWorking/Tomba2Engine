---
id: C010
kind: claim
status: falsified
created: 2026-07-28
tags: render
falsified_on: 2026-07-28
---

## Claim

FUN_800328EC is a third shared sprite writer with 6 overlay controllers behind it and NO native owner, so every effect that emits through it is invisible under pc_render.

## Evidence

codemap --addr 0x800328EC: no native owner. rec_dispatch call sites resolved to enclosing recompiled functions: 0x8013D454, 0x8012D9E8, 0x8012E868, 0x801346C0, 0x8013B118 (x2), 0x8010C1D8, plus two MAIN.EXE sites. Emitter contract decoded from ov_a01_gen_8012E868. 2026-07-28.

## What would falsify it

if a producer scope or tap is found that already routes 0x800328EC, or if all six controllers turn out to be cold in every area

## FALSIFIED 2026-07-28

WRONG FRAMING. FUN_800328EC is not a writer: its whole body is three instructions — zero the depth-cue IR0 at 0x1F800090 and tail into FUN_8002847C(model, 0, 0), which is the SAME four-corner writer Render::fxAnimSpriteRender already reproduces host-side. What was actually missing was DISPATCH for its controllers, not a producer for a new family. Its two 'unowned helpers' are equally benign: 0x800329E0 loads the scene-camera CRs and sets DQA/DQB (the family's standard setup) and 0x800317CC is RTPS + the SpriteAnchor::otKeyInRange gate, publishing OT key / SXY2 / MAC0 to the same scratchpad slots fx_ring.cpp already documents. Fixed by reusing emitAnimQuadRecords through Render::altSpriteEmit — about 40 lines, not a new family port. Relied on by: the docs/findings/render.md entry 'A SECOND shared sprite writer' (corrected in place) and portmap fx-sprite-writer-328ec (retargeted).

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
