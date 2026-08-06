---
id: I042
kind: instrument
status: trusted
created: 2026-08-06
---

## Instrument

PSXPORT_DEBUG=ropeline — the shockwave-ring line of Render::shockwaveRingRender (game/render/fx_line.cpp), which now carries a DENOMINATOR: spans drawn out of 7, spans rejected behind the camera, and the projected screen box those spans landed in

## Validated by

VALIDATED BOTH WAYS on real data, 2026-08-06, Tomba2 replays/bugs/bucket-softlock.pad, headless PSXPORT_GATE=1 pc_render.
POSITIVE: with the fixed producer the line reports spans=7/7 behind=0 and a screen box that tracks the guest's OWN lineprim packet vertices to ~1px on 8 sampled frames (f270/271/275/279/283/287/320/355) — e.g. f320 native (93.3,132.6)..(163.3,159.2) vs guest (93,132)..(165,159).
NEGATIVE: on the SHIPPED (broken) producer the SAME line reports spans=7/7 behind=0 with a DEGENERATE box — x0==x1 and y0==y1 to one decimal on every call, e.g. f279 screen=(127.8,162.4)..(127.8,162.4) — which is what a ring collapsed to a single point looks like, and is how the second root cause (Robj divided by 4096) was found in ONE run.
WHY IT EARNS ITS KEEP, and this is the lesson rather than the tool: the PREVIOUS version of this same line printed only node/scale/grey/position. It ran 152 times per replay and could not have shown either bug — a producer that draws nothing and a producer that draws correctly print the same line. Claim C036 spent a whole session on a zero this line could have explained. A diagnostic whose only content is 'I ran' is not an instrument.
BLIND SPOT, stated: the box is the union of the projected SPAN VERTICES, not of the rasterised stroke — it excludes the 0.5px stroke half-width and the (+2,+1) shadow copy, so a containment test against it must dilate by both. And it says nothing about whether the emitted quads survived the depth test.

## Known failure modes

(none recorded yet)
