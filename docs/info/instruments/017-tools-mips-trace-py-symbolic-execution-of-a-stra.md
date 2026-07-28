---
id: I017
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

tools/mips_trace.py — symbolic execution of a straight-line MIPS range

## Validated by

Answers 'what value does this store write, in terms of the inputs' by giving every register a symbolic expression and interpreting the range, instead of reading a 3-instruction window by hand. Built because hand-reading windows produced TWO wrong generative-rule claims in one session (the E680 angle: first 'iterated map a <- cos(a)', then 'sweep = counter << 10' — both wrong, the truth being a 5-entry table). VALIDATED ON TWO CASES WITH KNOWN GROUND TRUTH, one of which I had previously gotten wrong: (1) 0x8002B3A4's trig argument returns r4 = ((s2 << 10) + 0), matching the hand-derived 'angle = loopCounter << 10'; (2) 0x8002E680's loop angle returns r17 = lh[((a1 << 1) + 0x800A20A8)+0], matching the hand-derived table lookup at 0x800A20A8 indexed by the counter. It also resolved E680's 64 packet stores to real expressions (centre +/- offset, and a precomputed sp+52.. stack table) where Ghidra showed only rotating aliased temporaries. LIMITS: straight-line only (no branch following), so give it a loop BODY between branch targets; memory is symbolic, not simulated, so a value stored then reloaded shows as lh[...] rather than being folded.

## Known failure modes

(none recorded yet)
