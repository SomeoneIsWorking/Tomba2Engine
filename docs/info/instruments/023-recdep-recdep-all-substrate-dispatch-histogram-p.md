---
id: I023
kind: instrument
status: trusted
created: 2026-07-29
---

## Instrument

recdep/recdep-all substrate-dispatch histogram (PSXPORT_DEBUG)

## Validated by

Shown to produce the OTHER answer, not just a uniform one: 0x80146478 read 127,275 hits before OverlayGt3Gt4::submitBlock was wired and DISAPPEARED from the histogram entirely afterwards, with ovhit accounting for the same 127,275 on the native side. Also fixed 2026-07-29 (psxport b786f444): recdep-all alone previously armed nothing and printed nothing, which is indistinguishable from a run that dispatched nothing — either channel now arms and the dump emits on whichever channel is on. Re-validated standalone: 324 unique targets where it previously emitted zero lines.

## Known failure modes

(none recorded yet)
