---
id: I040
kind: instrument
status: DISTRUSTED
created: 2026-08-06
distrusted_on: 2026-08-06
---

## Instrument

PSXPORT_DEBUG=walk and PSXPORT_DEBUG=quadrtpt — BOTH STRUCTURALLY SILENT under PSXPORT_GATE=1

## Validated by

NOT a working instrument in the standard measurement mode, recorded here so its silence stops being read as a census. Both channels live inside native override bodies (Render::renderWalk @0x8003C048, QuadRtptSubmit::submitQuad @0x8003B320). PSXPORT_GATE=1 sets Game::psx_fallback=1 (native_boot.cpp:605) and override_registry.cpp:74 then runs e.gen(c) for EVERY registered address, so the native body never executes. Measured: PSXPORT_DEBUG=ovhit on replays/bugs/seesaw-weight.pad under PSXPORT_GATE=1 prints native=0 for all 482 registered addresses while the same run shows oracle=14 for 0x8003B320 and oracle=381 for 0x8003C048's neighbours — the addresses are reached, our code is not. Consequence: docs/port-map.md's 'PSXPORT_DEBUG=walk shows only 3 targets ever fire across the whole replay library' is UNMEASURED, not a negative result. Before trusting any quiet channel, check PSXPORT_DEBUG=ovhit for native=0 on the address that owns it.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-08-06

the channels themselves are not broken — their PLACEMENT is: both sit inside native override bodies, which PSXPORT_GATE=1 bypasses entirely. Any result of the form 'channel X printed nothing, therefore the thing does not happen' taken under GATE is void. Re-check anything that cited walk or quadrtpt for a negative.

> Every result this instrument produced is suspect until it is re-validated.
