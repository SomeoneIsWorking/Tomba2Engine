---
id: I022
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

single-instant screenshot A/B (warp <area>; skip 600; shot) as proof that a native render producer does or does not paint

## Validated by

NOT trustworthy as a NEGATIVE: it reported '0 px changed' for TWO working producers on 2026-07-28. Render::fxRingSpriteRender (0x80110C14) emits 21/21 items every frame, but at the sweep's sample instant the whole ring sits at screen y[240..395] on a 240-line frame — entirely below the bottom edge — so ON and OFF screenshots are byte-identical. Re-shot at skip 200 the SAME binaries differ by 202 px. A 0-px result therefore means 'not visible at this instant', never 'the producer does nothing'. Validate a negative by making the producer log its own SCREEN extent (fx_sprite.cpp's ring line) and shooting a frame where that extent is inside the viewport, or by a scale-up positive control.

## Known failure modes

(none recorded yet)
