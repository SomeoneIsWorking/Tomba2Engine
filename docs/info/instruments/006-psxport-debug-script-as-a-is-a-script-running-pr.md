---
id: I006
kind: instrument
status: DISTRUSTED
created: 2026-07-23
distrusted_on: 2026-07-23
---

## Instrument

PSXPORT_DEBUG=script as a 'is a script running' probe

## Validated by

MISLEADING: it only logs the NATIVE ScriptInterp::step. rec_dispatch(0x80041098) runs the substrate gen_func_80041098, which logs nothing — so a fully running script can produce ZERO lines. Measured 2026-07-23: 0 lines over 120 locked frames while the script cursor obj+0x6c was demonstrably armed at 0x80148AF4. Read the cursor at obj+0x6c and the progress byte at obj+0x70 instead.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-23

silent for scripts running on the substrate gen body; silence is not evidence of a stopped script

> Every result this instrument produced is suspect until it is re-validated.
