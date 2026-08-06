---
id: I044
kind: instrument
status: trusted
created: 2026-08-06
---

## Instrument

PSXPORT_ORACLE=1 as the substrate render reference (pure OT painter order: gpu_native.cpp forces is3d=0/bg=0 under game->oracle, so no native band/depth/wide/fps60 decision reaches the picture)

## Validated by

Ran it against BOTH classes. POSITIVE: areas 0/13/14 at the settled warp viewpoint each draw the full world (a0/a13/a14_oracle.png) and sit at the documented ~28k >8/255 pc-vs-psx baseline (28794/28238/28171 of 76800). NEGATIVE-DIRECTION control: ORACLE vs ORACLE+PAINTER=1 (PAINTER reproduces the pre-fix bg=0 sprite banding) differ by 13 px of 76800, so the oracle picture does not depend on the classification that broke the RENDER_PSX leg.

## Known failure modes

(none recorded yet)
