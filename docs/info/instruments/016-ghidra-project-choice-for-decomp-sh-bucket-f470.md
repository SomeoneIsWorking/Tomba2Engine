---
id: I016
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

Ghidra project choice for decomp.sh — bucket_f470 (RAM dump) vs proj

## Validated by

THE bucket_f470 PROJECT SILENTLY TRUNCATES MAIN.EXE-RESIDENT FUNCTIONS. It is imported from a 2MB RAM dump captured at one frame, and its auto-analysis gets function boundaries wrong in the 0x8002xxxx effect band: it renders whole bodies as one-line tail calls. Measured three times in one session — 0x8002ECD8 came back as ending in a call to Trig::rsin (impossible for an emitter), 0x8002E680 (1624 bytes, verified by raw scan) came back as a SINGLE line calling 0x80083E80, and the same class hit 0x8013E08C's vertex loads. Every one of those verdicts was wrong. Re-running the SAME addresses against the  project decompiles them correctly and completely (0x8002E680: full body with 25+ locals instead of one line; 0x8002ECD8: 91 lines instead of 72, and the missing 19 lines are the entire rsin/rcos argument computation that feeds the emit call). RULE: use  for MAIN.EXE-resident code (below ~0x80100000) and reserve the RAM-dump projects for OVERLAY-resident addresses that only exist in a dump. Cross-check any function whose decompile ends suspiciously early against the raw instruction stream.

## Known failure modes

(none recorded yet)
