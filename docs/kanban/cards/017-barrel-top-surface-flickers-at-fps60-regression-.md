---
id: 17
title: Barrel top surface FLICKERS at fps60 (regression from the #11 sort-key fix)
status: done
labels: [render, fps60]
created: 2026-07-22
updated: 2026-07-22
evidence: docs/reference/issues/issue12_13_pc_vs_oracle.png
---

USER 2026-07-22. The waterpump barrel top fixed in #11 is stable at 30fps but FLICKERS at fps60. This is a regression in what #11 shipped, so treat it as such - do NOT revert #11 (the black-face bug it fixed is worse and the user has objected to reverting working fixes); make the fix frame-kind-stable. MECHANISM TO CHECK FIRST: #11 works by RenderQueue::resolveKeyOrder snapping a face pair when the game's own sort key (game_sort_key in game/render/submit.cpp) contradicts the depth buffer, decided by a pairwise strictly-interior sample of both polygons. At fps60 the interp present RE-RUNS the world pass under a LERPED camera into an isolated sink. If the key inputs (SZ from the native projection, ZSF3/ZSF4 captured at the camview_publish frame-time choke) or the interior-sample test resolve differently between the real and the interpolated frame, the snap engages on one and not the other - which is precisely a per-frame flicker. Verify by capturing a real and an interpolated frame at the same camera and diffing the barrel region, and by logging 'debug keyord' on both frame kinds to see whether the same pair snaps. The known-weak part of #11 is that interior-sample test (documented residual: sub-sample sliver contests are missed) - a pair sitting near the sample threshold would flip between frame kinds.

**2026-07-22:** 2026-07-22 FIXED. Root cause was NOT the interior-sample test this card suspected. RenderQueue::flush finishes a real frame with resolveKeyOrder THEN sortQueue; Fps60::tier1Render finished the interpolated frame's re-rendered world with sortQueue alone, so kanban #11's authored-order snap ran on every real frame and no in-between frame. A/B measured at the barrel pixel (165,92) on the GATE leg with fps60dump over 12 logic frames: before, interp=(8,8,16) black cap and real=(40,152,248) blue water, alternating for all 12; after, both kinds (40,152,248) for all 24 presents. Fix: RenderQueue::finalize(core) = resolveKeyOrder + sortQueue, and BOTH queue-finishing sites call it (external/psxport 32270b8e). Repro used: the #11 barrel repro from docs/findings/render.md plus 'debug fps60dump'.
