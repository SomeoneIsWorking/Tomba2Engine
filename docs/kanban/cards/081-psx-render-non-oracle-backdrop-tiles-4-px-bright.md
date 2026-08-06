---
id: 81
title: psx_render (non-oracle) backdrop tiles: 4-px bright-cyan band at each tile's right edge in the RQ_BACKGROUND path
status: todo
labels: [render, psxport]
created: 2026-08-06
updated: 2026-08-06
---

**2026-08-06:** Surfaced by the kanban #78 fix (Render::backdropTexpagePublishTick), which correctly moves the guest's own 16x16 backdrop tiles from RQ_HUD to RQ_BACKGROUND on the PSXPORT_RENDER_PSX=1 leg. With them in the background band, each tile shows a 4-px bright-cyan band along its RIGHT edge.
MEASURED, area 0 free-roam, scratch/shots/psxref/a0_psx_fix.png, scanline y=60: sky is (120,176,216) with runs of (32,224,240)/(48,248,248) at x=8..11, 56..59, 72..75, 88..91, 104..107 — exactly the last 4 columns of each 16-px tile (tile x0 grid is -20,-4,12,28,...,108).
NOT GEOMETRY: PSXPORT_PRIMDUMP=3100:3102 shows the tiles abut exactly and carry exact 16-texel UV spans (x=-4..12 uv 112..127, x=12..28 uv 128..143, mode=0 4bpp, clut=(1008,250), semi=0, raw=0). 4 texels = one VRAM halfword at 4bpp, which is the shape of a sampling/fetch bug, not a quad-placement one.
ABSENT under PSXPORT_ORACLE=1 (a0_oracle.png is clean sky), where painter order forces bg=0 and the same tiles emit through the 2D-FG path instead. So the defect is specific to the RQ_BACKGROUND / RQ_OM_2D_BG emit path for guest 16x16 sprites in external/psxport (gpu_native.cpp:1031-1039 -> RenderQueue::emitOrQueue/emitItem).
NOT FIXED: the code is in external/psxport, which another agent was restructuring during this session — the tree was not touched. This is a psxport-side card.
Does not block the reference: use PSXPORT_ORACLE=1.
