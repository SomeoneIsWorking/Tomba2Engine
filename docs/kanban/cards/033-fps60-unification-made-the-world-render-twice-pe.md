---
id: 33
title: fps60 unification made the world render twice per logic frame — remove the now-dead guest-time world build
status: done
labels: [render, fps60, perf]
created: 2026-07-22
updated: 2026-07-23
---

Follow-up to the presentPass unification (#31). Now that BOTH presents are produced by presentPass(c,t) — the real frame being presentPass(c,1.0f) — the world prims that the guest-time pass builds into the live queue are never drawn: the world comes from the tier1 re-render on both presents, and mRqCur is consulted only for the verbatim non-tier1 remainder plus as the 'cur' endpoint of the capture slots. So the world is BUILT three times per logic frame and DRAWN twice. Measured cost of the second draw, headless, 3000 frames from newgame: 13.0s before the unification, 26.5s after — very close to 2x, so the world re-render dominates. The fix is not to undo the unification (it is pixel-exact and it is the architecture the user asked for twice); it is to stop building the world into the live queue at guest time, since that build now exists only to (a) populate mCamCur/mObjCur/mBgCur capture slots and (b) establish seq ordering for the merge — both of which can be had without submitting drawable prims. Doing that lands at ONE world build and TWO draws, i.e. cheaper than the pre-unification path per present and strictly more unified. Do not paper over the cost by special-casing the real frame back to a verbatim replay: that is the two-code-paths bug (#17) coming back.

**2026-07-23:** FIXED 2026-07-23. Guest-time world DRAW removed on tier1-eligible scenes via Fps60::mWorldCaptureOnly (fps60.h); world leaves run capture-only (keep sceneCam/projObj/bgScroll + the load-bearing host publishes camview_publish/proj_set_H/gpu_bg_texpage_set, skip projection+submit). Pixel-exact: 232 frame-pairs 0-diff (field/hut/dialog/seesaw), TFORCE 0/76800, barrel #11 blue on both kinds. ~20% faster (10.34s vs 12.98s, 3000f newgame). Files: game/render/render_walk.cpp, submit.cpp, external/psxport fps60.{cpp,h}. See docs/findings/render.md.
