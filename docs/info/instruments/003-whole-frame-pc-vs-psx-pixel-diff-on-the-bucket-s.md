---
id: I003
kind: instrument
status: DISTRUSTED
created: 2026-07-23
distrusted_on: 2026-07-23
---

## Instrument

whole-frame pc-vs-psx pixel diff on the bucket-softlock replay

## Validated by

measured: the same ~50092/76800 differing pixels with AND without the line producer present, because the two legs present a camera about 2 logic frames apart on this replay - it cannot resolve a 130 px producer

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-23

it reports ~50092/76800 differing px with AND without the change under test (the two legs' cameras are ~2 logic frames apart on this replay), so it can neither confirm nor deny a 130 px producer. Use an A/B of the producer on-vs-off plus the lineprim coordinate census.

> Every result this instrument produced is suspect until it is re-validated.
