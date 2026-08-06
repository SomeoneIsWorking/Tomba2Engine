---
id: I043
kind: instrument
status: trusted
created: 2026-08-06
---

## Instrument

psx_render-vs-pc_render present-shot comparison as the confirming EYEBALL for a native producer (capture the same present index on one binary, once with PSXPORT_RENDER_PSX=1 and once without, and look for the layer in both)

## Validated by

TRIED 2026-08-06 on the shockwave ring and it COULD NOT ANSWER — recording it so the next session does not spend the same time on it.
WHAT WAS RUN: one binary, present 320 of replays/bugs/bucket-softlock.pad, PSXPORT_VK_HEADLESS=1, both render modes, on the broken and the fixed producer. All four shots are 960x720.
WHAT IT SHOWED, and it is a real result: the psx_render shots from the BROKEN and the FIXED binary are BYTE-IDENTICAL (md5 694d0eef0af3062eb073ebe7ad03083d) — correct and useful, since a pc-side producer must not touch the psx picture, so the psx leg is purely the guest's own drawing.
WHAT IT COULD NOT DO: establish PIXEL CORRESPONDENCE between the two modes. The psx present composites the picture differently, so cropping the ring's own guest-space footprint (94,133)..(162,159) out of both and comparing gives two unrelated images; there is no leg-independent way to say 'the ring is HERE in the psx shot' from the picture alone. A diff between the modes is ~the whole frame, which instrument I005 is already DISTRUSTED for.
USE INSTEAD, and it is strictly stronger: the guest's OWN submitted screen coordinates from PSXPORT_DEBUG=lineprim on the psx leg, matched frame-by-frame against the native producer's projected box. That compares the two at the SUBMISSION boundary rather than after two different compositors have had a turn, and it gave ~1px agreement on 8 frames where this comparison gave nothing.

## Known failure modes

(none recorded yet)
