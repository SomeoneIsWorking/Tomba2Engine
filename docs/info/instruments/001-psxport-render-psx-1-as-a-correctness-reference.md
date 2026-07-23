---
id: I001
kind: instrument
status: DISTRUSTED
created: 2026-07-23
distrusted_on: 2026-07-23
---

## Instrument

PSXPORT_RENDER_PSX=1 as a correctness reference

## Validated by

PARTIAL: boot log confirms Render::psxRender=1 engages, so the leg is real. NOT validated as ground TRUTH — it shares exec state with the default leg, so it cannot detect an exec-state fault (see C001).

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-23

A THIRD failure mode, found while root-causing #59: on the DEFAULT exec leg, PSXPORT_RENDER_PSX=1 at boot produced a frame BYTE-IDENTICAL to pc_render (md5 a42d99bcde4d) for the item menu while logging Render::psxRender=1. Nuance the operator adds: that identity is ALSO explained by the native menu producer being pixel-faithful AND both legs receiving the same present-time global fade — so 'the flag did not engage' is NOT proven, only that the leg is USELESS as a correctness reference when a fault lives in a present-time stage both legs share. Either way: do not use RENDER_PSX on the default exec leg as ground truth. The only leg that showed the true picture was PSXPORT_GATE=1 + PSXPORT_RENDER_PSX=1 (recomp exec + psx render).

> Every result this instrument produced is suspect until it is re-validated.
