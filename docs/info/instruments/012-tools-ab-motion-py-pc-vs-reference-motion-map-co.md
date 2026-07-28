---
id: I012
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

tools/ab_motion.py — pc-vs-reference MOTION-map comparison

## Validated by

Compares each leg against ITSELF over time (per-tile 'did this change') and diffs the two maps, so it is immune to the rasteriser differences that make a raw pc-vs-psx pixel diff useless (I005). Validated by feeding it a case that MUST differ: at the AUTO_SKIP start spot it isolated ONE contiguous 22-tile block (x=224..300,y=0..96) moving 14-21/23 steps on the reference and 0/23 under pc_render, with 0 pc-only tiles — and a zoomed crop of that exact region shows a flying bird present on the reference and absent under pc_render (kanban #63). It also correctly reported NO missing motion anywhere else on the same screen, so it is not just flagging everything.

## Known failure modes

(none recorded yet)
