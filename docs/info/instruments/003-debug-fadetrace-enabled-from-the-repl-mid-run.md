---
id: I003
kind: instrument
status: DISTRUSTED
created: 2026-07-23
distrusted_on: 2026-07-23
---

## Instrument

debug fadetrace enabled from the REPL mid-run

## Validated by

SILENTLY DEAD when turned on after boot: ScreenFade::fadetrace latches mTraceOn once (mTraceOn<0 check) on the FIRST call, which happens during boot, so a later 'debug fadetrace' never prints. Turning it on via PSXPORT_DEBUG=fadetrace at boot printed 1632 lines for the same run. Same lazy-latch shape appears in other channels — check for 'static int x = -2' / 'if (m < 0) m = cfg_dbg(...)' before trusting an empty channel.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-23

lazy one-shot latch: the channel is dead unless enabled at boot (PSXPORT_DEBUG=fadetrace).

> Every result this instrument produced is suspect until it is re-validated.
