---
id: C004
kind: claim
status: holds
created: 2026-07-23
tags: 
---

## Claim

pc_render draws the game's ropes/chains/tethers natively; the only line emitter still without a producer is the ground ring shadow FUN_8013E08C

## Evidence

debug lineprim census over replays/bugs/bucket-softlock.pad f0..445 finds 3 line emitters; 2 share the leaf FUN_8013DD34, both ported (fx_line.cpp); native stroke count == guest stroke count at the matched frame; A/B with the dispatch disabled isolates 130 px (rope) and 69 px (tether) and the added pixels are exactly dst+F/4 (blend 3)

## What would falsify it

a lineprim census in another scene showing a 0x5E line packet from a fourth emitter, or a rope visible under PSXPORT_RENDER_PSX=1 and absent under pc_render at the same frame index
