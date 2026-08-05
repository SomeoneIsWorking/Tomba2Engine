---
id: I033
kind: instrument
status: trusted
created: 2026-08-05
---

## Instrument

PSXPORT_DEBUG=ringcensus — Render::ringNodeCensus (game/render/render_walk.cpp), the stunned-enemy ring/stars node census for kanban #72/#55

## Validated by

Validated BOTH directions in one run (walk-dust-puff.pad): prints the NEGATIVE with its denominator ('walked=132 type20=11 rings=0 (no ring node live this frame)') on frames with no ring, and the POSITIVE ('walked=156 type20=32 rings=1 [node=800EE730 ... owner=800FD328 stunbit=1 vis=1]') on 676 consecutive frames that had one — so it is not a tool that can only ever say 'none'. Cross-checked against the REPL 'ents' command in the SAME run: ents independently showed node 800EE730 with h(node+0x1C)=8002B7B0 rf(node+1)=1, matching the census exactly. It runs BEFORE fieldObjectsRender's 'node+1==0' skip and matches on node+0x18==0x8002B3A4 OR node+0x1C==0x8002B7B0, so it can see an INVISIBLE ring node — which every pre-existing type-0x20 diagnostic structurally cannot.

## Known failure modes

(none recorded yet)
