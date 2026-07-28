---
id: I021
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

22-area warp sweep (docs/areas.md recipe: newgame; skip 3000; warp N; skip 600) with PSXPORT_DEBUG=nofx — the coverage bridge between the replay library and the whole game.

## Validated by

Validated by producing an answer the replay library could not: the 15-replay nofx census was clean, and this sweep found EIGHT render fns with no producer, at most two per area, in areas 3/4/8/10/11/13/14/21 that no replay walks. Cross-checked against the RAM dumps: 0x8013B118 sits at node+0x18 of a type-0x20 vis=1 node in the area-4 capture, and 0x8012D9E8 likewise in area 1. All 22 areas exit 0 with zero abort/fatal/miss. LIMIT: it samples ONE instant 600 frames after warp, so trigger-gated effects still will not appear — it is a lower bound too, just a much better one.

## Known failure modes

(none recorded yet)
