---
id: I047
kind: instrument
status: DISTRUSTED
created: 2026-08-06
distrusted_on: 2026-08-06
---

## Instrument

REPL 'tp X Y Z' + 'rw 800e7eac 3' as a way to reach a reported CAMERA coordinate

## Validated by

BROKEN FOR THAT PURPOSE, measured 2026-08-06. (1) The user-facing HUD triple is the CAMERA readout 0x1F8000D2/D6/DA (overlay_glue.cpp:31-33 -> rml_overlay.setWorld); tp writes TOMBA's master position 0x800E7EAC/B0/B4 (Engine::devTeleportApply). Different quantity. (2) The 'confirm tp fired' readback rw 800e7eac reads back the very word tp wrote (MASTER_X = G+0x2C = 0x800E7EAC, cutscene_camera.h:47) - it can never report failure. (3) In area 14 it changes nothing: tp 29940 498 6928 vs tp 20161 -1923 8268 = 80/76800 px and a BYTE-IDENTICAL camera triple; walking 240 frames leaves 0x800E7EAC unchanged. Valid use: only as 'the area's default settled view', never as 'this coordinate'.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-08-06

It answers a different question than the one asked: tp writes Tomba's master position while the reported coordinate is the CAMERA readout, and the rw 800e7eac confirmation reads back tp's own write so it can never fail. In area 14 it moves nothing at all (80/76800 px, byte-identical camera triple).

> Every result this instrument produced is suspect until it is re-validated.
