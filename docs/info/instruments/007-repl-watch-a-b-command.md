---
id: I007
kind: instrument
status: DISTRUSTED
created: 2026-07-23
distrusted_on: 2026-07-23
---

## Instrument

REPL 'watch <a> <b>' command

## Validated by

Takes lo/hi bounds, NOT addr/length. 'watch 800E7E80 1' arms the range [800E7E80,80000001) and silently never fires. Correct form: 'watch 800E7E80 800E7E81'. Also needs PSXPORT_DEBUG=cw to print, and PSXPORT_CW_BT=1 for a host backtrace per store (which is what identified both deadlocked actors in kanban #60).

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-23

REPL 'watch <a> <b>' takes LO and HI addresses, not addr+len. 'watch X 1' arms an EMPTY range and silently never fires — a watch that never fires is indistinguishable from 'the address is never written', which is exactly the wrong conclusion. Use 'watch X X+n'.

> Every result this instrument produced is suspect until it is re-validated.
