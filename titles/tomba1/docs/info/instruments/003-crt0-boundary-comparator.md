---
id: I003
kind: instrument
status: trusted
created: 2026-08-27
---

## Instrument

`tools/compare_crt0_boundary.py` — independent-oracle/Lightrec CRT0 call-boundary comparator.

## Validated by

Two independent-oracle runs agreed byte-for-byte at the first call, all 35 compared fields agreed
with the recorded pre-migration execution, a forced `gp` mutation produced exactly one reported difference, and a
too-short oracle trace refused before a boundary.

## Known failure modes

The first-call boundary does not prove any later initialization, scheduler, frame, input, rendering,
or widescreen behavior. Those need their own running evidence.
