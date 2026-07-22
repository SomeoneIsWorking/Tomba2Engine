---
id: 25
title: pc_render area sweep: 24 of 32 areas sampled clean, coverage recorded
status: done
labels: [render]
created: 2026-07-22
updated: 2026-07-23
---

2026-07-22. Method (the one that works — the older GATE-vs-ORACLE compare is BLIND to tap-based producers because taps never fire under PSXPORT_GATE=1): ONE process on the DEFAULT leg, three shots A(pc,f) / B(psx,f+1) / C(pc,f+2) via the `renderpsx` REPL toggle, then count pixels that differ from BOTH pc neighbours while the two pc neighbours agree (scratch/abdiff.py) — that cancels camera motion, which a naive A-vs-B diff does not.

SAMPLED (all default leg, headless):
  - seaside replay (replays/bugs/seesaw-weight.pad) f6000: 90 px renderer-attributable of 76800.
  - hut exterior (replays/scene-transitions/hut-entry-alt.pad) f500/f535/f700.
  - dark-screen replay (replays/bugs/dark-screen-repro.pad) 15 points, f4000..f60000 — note this
    replay barely moves: 13 of 15 points are the same cliff, poor variety, do not rely on it.
  - PROVOKED at seaside f6000: hold-square 12 frames, tap select / triangle / circle, pause menu
    (start) 6 points. Triangle item menu and the start menu are PIXEL-IDENTICAL between the two
    renderers (0 diff) — but see #21: both are equally WRONG, the oracle shows an opaque panel.
  - AREA WARP (REPL `warp N` — the real coverage lever, 32 areas): 1-21 and 23-27 entered.
    Everything under 100 px renderer-attributable except four scenes at 500-2100 px, and each of
    those inspected by eye is texture/dither speckle spread over the whole image (y10, y11, z15,
    x12 D-overlays in scratch/screenshots/layergap/), not a missing layer.

NOT sampled: areas 28-31 (session budget), and area 22 (aborts — see the 0x80109200 card).

FOUND, not a missing layer, still real: x12 (a temple interior) draws its CEILING BEAM band warped
and displaced under pc_render where psx_render draws it straight — scratch/screenshots/layergap/
x12top_A.png vs x12top_B.png. Filed separately.

CONCLUSION: the missing-layer class is NOT broadly distributed across areas. The remaining reported
gaps (#14 #15 #18 #19 #21) are all TRIGGER-gated — they need a specific action (long hold-attack, a
pickup, a dialog, a menu), not a specific place. Sweeping areas will not find more of them.

**2026-07-23:** CONCLUSION FALSIFIED 2026-07-23. This card's headline ('24 of 32 areas sampled clean; the missing-layer class is NOT distributed across places') rested on a pc-vs-pc comparison: the sweep's 'psx reference' came from a BARE renderpsx REPL command, which only PRINTED the flag and set nothing (fixed in external/psxport repl.cpp). So no area was ever compared against psx_render.

I re-ran the sweep with a REAL reference (tools/warpsweep.sh, second run under PSXPORT_RENDER_PSX=1 at the same frame index, all 22 areas 0..21). Result: the missing-layer class IS distributed. Real pc_render gaps found, each with foreground state identical between legs so it is a genuine renderer delta, not motion or desync:
  - MISSING far BACKGROUND/SKY plane: areas 10, 11, 14, 21 -> new card #42. pc draws black where psx draws the cave/night-sky/waterfall/sky backdrop.
  - MISSING HUD minimap: areas 2, 7 -> new card #43.
  - MISSING central vortex effect: area 15 -> new card #44.
Baseline-clean (background present both, only the generic ~23k-px native-shading delta): areas 0,1,5,6,8,9,12,13,16,17,18,19,20.
Non-results (cold-warp artifacts, kanban #36, NOT renderer bugs): area 3 both legs pure black; area 4 the two legs landed in different game states (snow vs starfield, health wheel differs) so non-comparable.

Also wrong on a second count, as docs/areas.md already records: '32 areas' does not exist — the game has 22 areas, 0..21. Evidence: scratch/screenshots/warpsweep/s.report + s*_{pc,psx}.png. Full write-up in docs/findings/render.md.
