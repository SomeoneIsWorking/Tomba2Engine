---
id: I046
kind: instrument
status: DISTRUSTED
created: 2026-08-06
distrusted_on: 2026-08-06
---

## Instrument

PSXPORT_PRIMAT [primat-rq] raw log as 'which producer covers this pixel'

## Validated by

DISTRUSTED AS-IS: its point-in-triangle test ((w0>=0&&w1>=0&&w2>=0)||(all<=0)) is TRUE at every pixel for a DEGENERATE triangle, and this path tees GT3s as quads with v3==v2. Measured 2026-08-06 (kanban #77 T2): at probe (160,40) frame 3940 the raw log listed FFFF0001/800FB218 hits whose own xy[] is nowhere near the probe, which is how 'dbgnode=FFFF0002 draws area 14's water wall' got recorded; the real producer 800ED960 was in the same log. VALIDATED FORM: pipe through tools/primat_filter.py (rejects degenerate tris, prints denominators, ships --selftest that must reject a degenerate and accept a real quad). Filtered result at three probes: 2 genuine covers each, all 800ED960.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-08-06

Degenerate-triangle false positives: its point-in-triangle test is TRUE at every pixel when a triangle has coincident vertices, and this path tees GT3s as quads with v3==v2. It named the WRONG producer for kanban #77 T2 (FFFF0002 instead of 800ED960). Use ONLY through tools/primat_filter.py, which rejects degenerate covers and prints denominators.

> Every result this instrument produced is suspect until it is re-validated.
