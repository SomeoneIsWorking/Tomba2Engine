---
id: I012
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

tools/ab_motion.py — pc-vs-reference MOTION-map comparison

## Validated by

Compares each leg against ITSELF over time (per-tile 'did this change') and diffs the two maps, so it is immune to the rasteriser differences that make a raw pc-vs-psx pixel diff useless (I005). Its original must-differ validation isolated ONE contiguous 22-tile block (x=224..300,y=0..96) moving 14-21/23 steps on the reference and 0/23 under pc_render, with 0 pc-only tiles; the crop showed the then-missing bird (kanban #63). A fresh 32-frame true-interpreter/software-GPU sweep on 2026-08-21 demonstrated the other answer after intervening renderer/behaviour repairs: the bird moved on both legs and the old block was absent, with only three isolated low-count tile differences. It also correctly reported no missing motion elsewhere in the original screen, so it is not just flagging everything.

## Known failure modes

(none recorded yet)
