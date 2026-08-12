---
id: 86
title: SBS-full driven by REPL 'newgame' (no AUTONAV) dies on an unmapped read in Render::fieldObjectsRender under renderAttract
status: backlog
labels: [bug,verification]
created: 2026-08-12
updated: 2026-08-12
---

2026-08-12: 'newgame; run 200' piped to the REPL with PSXPORT_SBS_MODE=full aborts fail-fast on UNMAPPED RAM read8 @ 0x07035D41 from Render::fieldObjectsRender <- sceneNative <- renderAttract <- Engine::drawOTag, on core A. The documented gate legs (AUTONAV=combat / AUTONAV=1 WATCH_CUT) are byte-exact on the same binary, and plain pc_faithful with the same REPL script exits 0, so this is specific to SBS + REPL-driven newgame reaching ATTRACT-mode pc_render. NOT established whether it predates 38cec620 — the discriminator (same run on an unmodified substrate) was not paid for, because the documented legs answered the gating question. If SBS is ever driven by REPL script, root-cause this first.
