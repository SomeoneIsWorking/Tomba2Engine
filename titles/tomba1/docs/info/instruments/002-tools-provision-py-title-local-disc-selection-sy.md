---
id: I002
kind: instrument
status: trusted
created: 2026-08-27
---

## Instrument

tools/provision.py — title-local disc selection, SYSTEM.CNF boot-path, and verified-publication instrument

## Validated by

tests/test_provision.py exercised 8 shipping-path cases: an agreeing synthetic disc published, while ambiguous/missing selection, malformed/multiple BOOT records, a wrong target, and altered executable produced the opposite refusal/mismatch and did not publish or replace output.

## Known failure modes

The instrument proves selected-disc boot provenance and byte identity, not startup semantics, a
running frame, input, gameplay, or widescreen correctness.
