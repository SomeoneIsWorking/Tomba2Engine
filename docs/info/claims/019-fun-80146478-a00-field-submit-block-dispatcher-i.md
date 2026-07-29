---
id: C019
kind: claim
status: holds
created: 2026-07-29
tags: render
---

## Claim

FUN_80146478 (A00 field submit-block dispatcher) is natively owned and byte-exact: OverlayGt3Gt4::submitBlock produces state identical to the recompiled body over 1500 frames of real gameplay

## Evidence

SBS full, PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad, PSXPORT_SBS_EXIT_FRAME=1500: 50/50 checkpoints A/B-identical, zero divergence. Coverage is proven not assumed — PSXPORT_DEBUG=ovhit reports 0x80146478 native=76378 oracle=76378, and both leaves the same, so the compare demonstrably executed the new code on both legs. Commit d20e2b4.

## What would falsify it

any SBS divergence whose call chain passes through 0x80146478/0x801465EC/0x801467BC; or an ovhit run where 0x80146478's native and oracle counts differ (that is a control-flow divergence the RAM compare can miss); or the seesaw-weight replay ceasing to reach the A00 overlay, which would make the 1500-frame window vacuous for this address
