---
id: I011
kind: instrument
status: DISTRUSTED
created: 2026-07-28
distrusted_on: 2026-07-28
---

## Instrument

replays/bugs/bucket-softlock.pad as a post-pickup 'does the cutscene complete' probe

## Validated by

DISTRUSTED for anything after f468. pad_decode shows the LAST button in the whole 1764-frame capture at f468; the pickup dialog opens ~f470 and every frame after is PAD_NONE. The dialog's advance is gated on the PRESSED edge mask 0x800E7E68 & the confirm mask 0x1F800174 (=0x2000, Circle), so a capture with no input CANNOT show the box advancing — it reports 'parked forever' whether or not the game is stuck. Proof it was lying: on the same replay, one 'tap o' over the debug server closed the box, the script resumed 801485A4->800A3E38->800A3E60, cut-mode 0x1F800137 cleared, and the player moved again. Still VALID as a probe for everything up to and including the pickup trigger.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-28

No input after f468 in a 1764-frame capture, while the dialog's advance is gated on a PRESSED-edge button mask. It reports 'parked forever' regardless of whether the game is stuck. Valid only up to the pickup trigger.

> Every result this instrument produced is suspect until it is re-validated.
