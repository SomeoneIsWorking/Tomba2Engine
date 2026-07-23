---
id: 56
title: SYSTEMIC: pc_render has NO line-primitive producer — every GP0 line is invisible (rope, fishing line)
status: todo
labels: [render, bug]
created: 2026-07-23
updated: 2026-07-23
evidence: docs/reference/issues/issue56_fisherman_line_missing.png
---

SYSTEMIC. USER 2026-07-23 reported the bucket's ROPE (#54) and a fisherman's fishing LINE missing under pc_render, in separate scenes. They are the same defect, and the cause is structural rather than per-effect.

MEASURED (grep, 2026-07-23): the GP0 interpreter HANDLES line primitives — gpu_native.cpp:991 `else if (op >= 0x40 && op <= 0x5F)` covers flat/gouraud single lines AND poly-lines, collecting an N-vertex list and drawing each segment. That is the psx_render path, which is why lines appear under PSXPORT_RENDER_PSX=1 and in the user's reference captures.
But the NATIVE RENDER QUEUE HAS NO LINE PRIMITIVE AT ALL: render_queue.{h,cpp} and game/render/submit.cpp contain no line path — the queue deals in triangles/quads (nv=3/4) via emitOrQueue / drawWorldQuad / push2dQuad. Since the break-first rebuild pc_render does not walk the guest OT, every GP0 line the guest emits is simply never drawn under pc_render.

SO THIS IS ONE GAP WITH MANY SYMPTOMS, not a series of missing effects. Anything thin and line-drawn is invisible: the bucket's suspension rope (#54), the fisherman's line, and plausibly other cables/wires/chains/whiskers not yet noticed. Expect more sightings; they should be attached here rather than filed separately.

THE FIX: give the render queue a LINE primitive and a native producer for it.
  1. RE the emitters. `debug otattr` on a scene with a visible line (the fisherman, or the bucket) attributes the op-0x40..0x5F packets to their emitting fn AND caller. Confirm whether they come from one shared line emitter or several.
  2. Add a line kind to the queue. A PSX line is a 1px-wide segment between two screen points with flat or gouraud colour and optional semi-transparency. Simplest faithful native form is to expand each segment host-side into a quad (2 triangles) of the correct width in screen space, submitted through the existing emitOrQueue path with real depth — no new rasterizer needed. Keep the expansion in the PRODUCER, not in the queue, so the queue stays quads-only.
  3. PORT the emitters per the USER directive (a tap is not acceptable): read the endpoints from the emitting object's own state, project natively with projComposeCamera(sceneCam) so the line lerps at fps60, emit with has_xyf=1. game/render/fx_sprite.cpp and fx_dust.cpp are the templates for a type-0x20 node producer.

WATCH OUT — a known past defect in this exact area: docs/findings (memory `tomba2-gfx-fixed-polyline`) records that gp0_len once MISSED variable-length poly-lines, walking off the end and clobbering VRAM, and that a differ running on corrupt VRAM then reported a FALSE 'faithful'. Poly-lines are variable-length (terminated, not counted). Do not assume 2 vertices.

VERIFY against a SECOND run with PSXPORT_RENDER_PSX=1 at the same frame index — never a bare mid-scene `renderpsx` toggle (kanban #41 proves it silently returns a pc frame).
