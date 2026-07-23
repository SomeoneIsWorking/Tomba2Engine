---
id: C002
kind: claim
status: holds
created: 2026-07-23
tags: 
falsified_on: 2026-07-23
reconfirmed: 2026-07-23
---

## Claim

SBS is 0-diff on main

## Evidence

cited from runs predating 2026-07-23

## What would falsify it

a rebuild that changes exec order, or a run on a DIFFERENT binary than the one measured

## FALSIFIED 2026-07-23

kanban #50 proved SBS is RED on main and was already red before its change (built the same tree twice, both diverge identically at f169). Any pre-2026-07-23 0-diff citation is stale.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.

## Re-confirmed 2026-07-23

Operator-verified on the applied subpart_walk fix: SBS-full seesaw leg, PSXPORT_SBS_NOPAUSE=1, ZERO sbs-div lines, 'A/B identical' through f19140 (run ended on my 400s timeout, not a divergence). Was first-diverging at f169/0x801FE808. Cause: Render::subPartWalk held its walk state in C++ locals while its still-substrate callee func_800803DC spills the guest s0/s1 into its own frame — so core A spilled stale registers. Fixed by mirroring s0..s4 (the LIVE-REGISTER LAW).
