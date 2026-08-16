---
id: 99
title: fps60=0 and fps60=1 are two different renderers — unify so the only difference is the extra lerp present
status: done
labels: [render, fps60, architecture, debt]
created: 2026-08-16
updated: 2026-08-16
---

USER, 2026-08-16: "fps60 and regular should be rendering the same thing and should work the same underneath and again this has been repeated a million times, the only difference would be whether to add the extra lerp frames or not." Same directive as 2026-07-15 ("no difference between real and interpolated frames aside from lerp") and 2026-07-22 ("there should be just one site") — BOTH of which unified the two frame KINDS within fps60=1, and neither of which unified the two CONFIGS.

WHY THIS IS THE ROOT CARD, not a nice-to-have: every render bug fixed on 2026-08-16 is a child of this split.
  - kanban #94/#35: the panel family dropped at fps60=1 only (rq_capture overwrote per flush; at fps60=0 flush emits directly).
  - zfightScan/rqhist scan the FLUSH queue, which at fps60=1 is not what gets drawn -> the z-fight finder reports fight=0 from a denominator of ZERO prims on every outdoor scene.
  - the 2026-08-14 painter-object layer is unreachable in the shipped config: its only call site is in emitQueue, which fps60=1 never reaches.

MEASURED DIVERGENCE (three layers deep, in order of discovery):
  1. PRESENT PATH. fps60=0: flush() -> emitQueue() -> guest's gpu_present. fps60=1: flush() -> rq_capture() (early return) -> frame_commit -> present_vk -> presentPass. frame_commit itself early-returns when the tier is off.
  2. WORLD EMISSION TIME. render_walk.cpp's mWorldCaptureOnly gates on mods.fps60, so the world is emitted at GUEST time at fps60=0 and at PRESENT time (tier1Render into mSink) at fps60=1. Different code builds the world in the two configs.
  3. CAPTURE CHOKES. Fps60::sceneCam / bgScroll / projObj only record their cur-slots 'if (active())'. So at fps60=0 there is no camera history at all.

ATTEMPTED AND BACKED OUT 2026-08-16 (do not re-tread blindly): unifying (1) alone leaves the cliff BLACK at fps60=1 with TFORCE=1, because the field world is not in the capture at all — it is capture-only at guest time. Unifying (1)+(2) without (3) renders the cliff as bare sea+sky bands at fps60=0, because tier1Render runs with an uncaptured (zero) camera — the same picture as the R22 camera bug. Both measured; tree reverted to the committed state and verified pixel-identical.

SCOPE: 5 active() guards in fps60.cpp, the mWorldCaptureOnly gate in render_walk.cpp, the mods.fps60 branch at game_tomba2.cpp:131, and the flush branch in render_queue.cpp. The acceptance gate is pixel-identity of the REAL present between the two configs across the four panel/scene replays, in BOTH directions, plus a perf measurement (unification makes fps60=0 build its world at present time, which it does not do today).

Evidence in the code that t=1 is the right unification point: with PSXPORT_FPS60_TFORCE=1 the interpolated present is 0/76800 px identical to the real present over 3 moving frames, against a validated positive control (TFORCE=0 -> ~45k px).

**2026-08-16:** DONE 2026-08-16, psxport 7713ec42. The invariant is written down in external/psxport/docs/one-renderer.md and pointed at from psxport's CLAUDE.md, including the table of branches that remain legitimate — a branch outside that table means the configs have diverged again. Acceptance gate met: the REAL present is pixel-identical between the two configs, with the check shown to fail as well as pass (cliff and start DIFFER before, IDENTICAL after; hut and menu were identical throughout, which is why a two-scene gate would have passed for the whole period the bug was live). Measured cost recorded, not waved off: fps60=0 goes 2.5s->3.5s per 900 headless frames (+40%, ~294fps against a 30fps target), fps60=1 unchanged. Direction chosen deliberately: building the world at guest time instead would be free here but would undo #33 and cost the DEFAULT config a second world draw per frame.
