---
id: I004
kind: instrument
status: DISTRUSTED
created: 2026-07-23
distrusted_on: 2026-07-23
---

## Instrument

pad replay (PSXPORT_PAD_REPLAY) as a CROSS-LEG comparison

## Validated by

INVALID and must not be used: .pad files are frame-indexed from boot and recomp_path / PSXPORT_PC_SKIP=0 consume a different number of boot frames, so the same pad lands somewhere else entirely on each leg. Measured 2026-07-23 on replays/bugs/sequence-softlock-2.pad: at f1200 pc_skip is inside a hut dialog while PSXPORT_GATE=1 and PSXPORT_PC_SKIP=0 are both out in the village (scratch/screenshots/{n_1200,o_1200}.png). A pad replay is only valid for the exec config it was captured in.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-23

frame-indexed pads desync across exec legs; never use one to compare recomp_path vs pc_skip

> Every result this instrument produced is suspect until it is re-validated.
