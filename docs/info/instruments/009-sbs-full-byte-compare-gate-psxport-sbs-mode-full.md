---
id: I009
kind: instrument
status: trusted
created: 2026-07-23
---

## Instrument

SBS full byte-compare gate (PSXPORT_SBS_MODE=full)

## Validated by

PSXPORT_SBS_CANARY=1500 flips one byte on core A only at f1500; the gate reports the divergence THAT frame at exactly 0x800E7EAC with both cores' last-writer. Built-in self-test — re-run any time, especially when a long-red gate suddenly goes green.

## Known failure modes

(none recorded yet)
