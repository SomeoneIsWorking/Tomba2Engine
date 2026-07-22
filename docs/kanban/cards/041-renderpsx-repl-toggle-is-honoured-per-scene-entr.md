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
