---
id: C052
kind: claim
status: holds
created: 2026-08-21
tags: render,house
depends: psxport.pin, external/psxport/runtime/psx/render_queue.cpp#RenderQueue::resolveKeyOrderFaces, external/psxport/runtime/psx/ot_lifo_depth.cpp#rq_apply_ot_lifo_depths
---

## Claim

House-on-the-Point wall decor reaches the current Native queue and presents visibly under OT-LIFO ordering

## Evidence

2026-08-21: exact house-on-the-point.pad faithful Native A at f3000 on psxport 2b5ef7b5 captured all historical left-wall hangings, blue/teal items, trophy and talk prompt. PSXPORT_DEBUG=keyord counted 467/467 keyed world faces; node 0x800FD748 supplied seq 76..343 faces, 121 faces were snapped and 96 faces used same-bucket OT-LIFO ties. The paired software B was outside and was explicitly not used.

## What would falsify it

a same-state current Native replay at the historical viewpoint omits the decor, node 0x800FD748 is absent from the shipping queue, or disabling/changing the shared OT-LIFO owner leaves the final picture unchanged
