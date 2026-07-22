---
id: 38
title: In-game Options sub-page renders without its full-screen dark-blue backdrop
status: todo
labels: []
created: 2026-07-23
updated: 2026-07-23
---

**2026-07-23:** 2026-07-23 (found while fixing #35; PRE-EXISTING, not caused by it). Repro: run 300; tap start; run 60; tap cross; run 60 — the 'Select Options / Messages / Sound / Screen adjust / Controls' page shows the live field through it; the psx_render leg has the backdrop. Evidence scratch/screenshots/c35/opt_pc.png vs opt_psx.png; otattr that frame (scratch/screenshots/c35/otattr2.txt) shows packet [2047] op=0x38 (POLY_G4 full-screen gradient = FUN_8007FC24) as the FIRST packet of the walk, attributed fn=0x8010810C. kanban #7 fixed exactly this for the TITLE/DEMO path only: Render::optionsPageNative + Render::optionsBackdrop (game/render/render_options.cpp) are driven from Render::renderTitle's stage-0x801062E4 / sm[0x48]==6 branch, so nothing invokes them when the same five page builders run under the IN-GAME 0x8010810C dispatcher. Text/cursor/footer glyphs come from the global Font taps and are fine; only the backdrop (and, on page 3, FUN_8007FCC8's boxes) lacks a producer on the in-game path.
