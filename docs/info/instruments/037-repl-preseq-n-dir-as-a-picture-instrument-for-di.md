---
id: I037
kind: instrument
status: DISTRUSTED
created: 2026-08-06
distrusted_on: 2026-08-06
---

## Instrument

REPL `preseq <N> <dir>` as a picture instrument for DISPLAY-PASS producers (fx_line's ropes, and by construction any producer the render walk defers to the present-time re-render)

## Validated by

NOT VALIDATED — CAUGHT LYING on first use, 2026-08-06. preseq dumps via gpu_vk.cpp dump_to() -> readback_vram(), a 320x240 VRAM readback, NOT the presented picture. A/B legs built from the same tree, one with fx_line.cpp's producers deleted and one with the rope stroke widened to 24px and forced to PURE WHITE (598 draws logged in both), diffed 0 changed pixels of 76800 examined across all 12 present passes. An instrument that cannot see a 24px white line across a 320x240 frame cannot certify that a 2px grey one is absent. USE INSTEAD: PSXPORT_PRESENT_SHOT_AT -> gpu_vk_present_shot(), which downloads s_present_img (960x720 headless sink) and DID resolve the same rope at 1584/1197/927 px. preseq remains fine for its documented purpose (30Hz oscillation over VRAM composites) — the defect is using it to answer 'did this producer draw'.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-08-06

Recorded as distrusted at creation — it was caught lying by its own negative control (a 24px pure-white rope, 598 draws, 0 changed pixels of 76800). Kept in the registry rather than deleted so the next agent reaching for preseq to answer 'did my producer draw' finds the failure instead of repeating it.

> Every result this instrument produced is suspect until it is re-validated.
