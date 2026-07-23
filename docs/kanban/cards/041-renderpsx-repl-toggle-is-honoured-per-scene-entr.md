---
id: 41
title: renderpsx REPL toggle is honoured per SCENE ENTRY, not per frame — a mid-scene flip does not repaint
status: todo
labels: [render]
created: 2026-07-23
updated: 2026-07-23
---

Measured 2026-07-23 while closing #26 (area 12, replay seesaw-weight). 'renderpsx on' issued at f3621 then 'run 1' + shot gives a frame BIT-IDENTICAL (0/76800) to the pure-pc f3622, even though the flag reads back 1 ([repl] Render::psxRender = 1). Issuing the SAME command BEFORE 'warp 12' gives a frame bit-identical (0/76800) to the boot-PSXPORT_RENDER_PSX=1 frame, and 23091 px from the pc frame. So the switch latches at scene entry.

CONSEQUENCE: ground truth #1 in docs/gfx-debug.md (the in-process toggle) silently returns a pc frame when flipped inside a loaded area. Combined with the bare-'renderpsx'-is-a-no-op defect (fixed), this is what manufactured #26 — see docs/findings/render.md. Until this is root-caused, any renderer A/B must take its reference from a SECOND RUN with PSXPORT_RENDER_PSX=1 at boot (tools/warpsweep.sh does that).

NOT root-caused: Engine::drawOTag (game/game_tomba2.cpp) branches on c->rsub.mode.psxRender() every frame, so something upstream must latch per scene entry. Note findings/render.md records the toggle DID change the picture mid-scene in other contexts (#14 mace spikes, the 'run 2 after toggling' rule), so the latch may be specific to what a warp reloads — that is the thing to pin down.

**2026-07-23:** 2026-07-23 ROOT-CAUSE CHAIN NARROWED to one function (operator, from code reading — not yet fixed). (1) REPL  ALWAYS reads the VK target via gpu_vk_shot (gpu_vk_enabled() is always 1 — repl.cpp:198), never the software s_vram. (2) drawOTag (game_tomba2.cpp:164) branches live on psxRender() every frame: psx branch rasterizes the guest OT into s_vram (gpu_dma2_linked_list) and does NOT call renderScene(), so the NATIVE GEOMETRY BATCH is empty; pc branch calls renderScene() which fills the batch. (3) present_window()->blit_src(s_vram)->gpu_vk_present() both UPLOADS s_vram AND renders the native batch into the VK target. So the latch lives in gpu_vk_present's upload_vram + render_geom interaction: this is the SAME mechanism as kanban #20 (render_geom CLEARS s_vram_tex to black when the batch is empty). NEXT SESSION: read gpu_vk_present / render_geom in external/psxport/runtime/recomp/gpu_vk.cpp and determine, with an empty batch (psxRender mode), whether the uploaded s_vram survives to the VK target or is clobbered, and whether that decision reads the LIVE psxRender flag or a value latched at scene entry. Likely fix: in psxRender mode present the uploaded s_vram (skip the empty-batch clear), keyed on the live flag. This is a present-path fix (#20 sibling), do it deliberately with fps60dump/shot A/B, not a rushed edit. Until fixed, ground-truth #1 protocol stays: take the psx reference from a SECOND boot run with PSXPORT_RENDER_PSX=1 (tools/warpsweep.sh / render_cmp.py already do).
