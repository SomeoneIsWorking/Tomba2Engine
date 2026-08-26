---
id: I001
kind: instrument
status: trusted
created: 2026-08-26
---

## Instrument

`tools/verify_executable.py` — title-local whole-file and PS-X header identity verifier.

## Validated by

The shipping path accepts an agreeing synthetic PS-X executable, rejects a payload mutation by SHA-1,
rejects a changed entry word, rejects a renamed executable, and refuses a header-short input. On the
real measured `SCUS_942.36`, it accepts all 15 facts and rejects a one-byte-altered scratch copy. The
selftest reports its positive/negative denominator.

## Known failure modes

Executable identity alone cannot prove the disc's `SYSTEM.CNF`, boot behavior, a rendered frame,
input, gameplay, or widescreen correctness. Those are separate frontier steps.
