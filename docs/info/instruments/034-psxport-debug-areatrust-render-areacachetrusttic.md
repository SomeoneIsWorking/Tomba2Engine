---
id: I034
kind: instrument
status: trusted
created: 2026-08-05
---

## Instrument

PSXPORT_DEBUG=areatrust — Render::areaCacheTrustTick (game/render/render_walk.cpp): per-world-frame print of the two area-cache trust latches AND the two guest bytes they latch on (SCENE_ENT_TABLE+6, PARALLAX_BG_SM+0x10), so 'the scene table / backdrop is missing' can be told apart from 'it was never allowed to draw', and a sceneTable=0 negative shows whether the reset it waits for has already gone by.

## Validated by

Run against BOTH classes on 2026-08-05: it printed 0/0 on the leg whose picture was near-black (7627/76800 nonblack) and 1/1 on the leg whose picture was complete, with the guest bytes identical on both — so it discriminates the failing case from the passing one and is not merely echoing guest state. It also prints on every world frame with no gating on the value, so an all-zero reading cannot be silence.

## Known failure modes

(none recorded yet)
