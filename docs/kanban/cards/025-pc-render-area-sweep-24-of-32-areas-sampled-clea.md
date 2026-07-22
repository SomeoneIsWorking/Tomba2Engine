---
id: 25
title: pc_render area sweep: 24 of 32 areas sampled clean, coverage recorded
status: done
labels: [render]
created: 2026-07-22
updated: 2026-07-22
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
