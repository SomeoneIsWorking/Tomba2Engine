---
id: I045
kind: instrument
status: DISTRUSTED
created: 2026-08-06
distrusted_on: 2026-08-06
---

## Instrument

PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1 as a render reference — DO NOT TRUST for 'does vanilla draw X'

## Validated by

CAUGHT LYING 2026-08-06 (kanban #78). It walks the guest OT but hands every prim to the native RenderQueue, which re-bands it (layer = is3d ? RQ_WORLD : (bg ? RQ_BACKGROUND : RQ_HUD), painted low->high). Sprite 'bg' needs sprite_is_bg_texpage, published only by a pc_render producer that this leg never runs. Measured area 0 f3100 with PSXPORT_PRIMDUMP: 972 prims = 617 is3d=1 world polys + 355 sprites, 355/355 bg=0 — all 352 backdrop tiles went to RQ_HUD and painted over the world, so the capture read 'sky and sea only' and would have been read as 'vanilla culls this'. The tile banding is FIXED (Render::backdropTexpagePublishTick) and this leg draws the world again, but its fidelity still depends on native producers having run and it still shows a 4-px cyan seam per backdrop tile (kanban #81). I044 was later distrusted too: use I051 for rasterization or I053 for interpreter-state comparison, within their declared scopes.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-08-06

Produced a false negative that nearly closed kanban #77 as 'vanilla culls this geometry': it drew NO world geometry at all, so it could not have produced the failing answer. Root cause and fix in docs/findings/render.md (kanban #78). I044 was later distrusted for sharing host shader semantics; neither is a correctness reference.

> Every result this instrument produced is suspect until it is re-validated.
