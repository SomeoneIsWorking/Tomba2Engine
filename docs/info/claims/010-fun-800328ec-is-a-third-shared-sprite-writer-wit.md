---
id: C010
kind: claim
status: holds
created: 2026-07-28
tags: render
---

## Claim

FUN_800328EC is a third shared sprite writer with 6 overlay controllers behind it and NO native owner, so every effect that emits through it is invisible under pc_render.

## Evidence

codemap --addr 0x800328EC: no native owner. rec_dispatch call sites resolved to enclosing recompiled functions: 0x8013D454, 0x8012D9E8, 0x8012E868, 0x801346C0, 0x8013B118 (x2), 0x8010C1D8, plus two MAIN.EXE sites. Emitter contract decoded from ov_a01_gen_8012E868. 2026-07-28.

## What would falsify it

if a producer scope or tap is found that already routes 0x800328EC, or if all six controllers turn out to be cold in every area
