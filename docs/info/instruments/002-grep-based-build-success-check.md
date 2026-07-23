---
id: I002
kind: instrument
status: DISTRUSTED
created: 2026-07-23
distrusted_on: 2026-07-23
---

## Instrument

grep-based build success check

## Validated by

NOT VALIDATED - see distrust note

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-23

grep -E ' error ' cannot match gcc's file:line:col: error: (no space before the colon), so a FAILING build read as passing and every test afterwards ran a stale binary. Gate on the process exit status, never a grep alone.

> Every result this instrument produced is suspect until it is re-validated.
