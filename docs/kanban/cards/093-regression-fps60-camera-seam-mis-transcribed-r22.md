---
id: 93
title: REGRESSION: fps60 camera seam mis-transcribed R22 — pc_render drew only the backdrop, camera read as broken
status: done
labels: [bug,render,camera,fps60,regression]
created: 2026-08-16
updated: 2026-08-16
---

USER 2026-08-16: "camera acting weird and some fisherman are absent".

Root cause: the fps60ReadSceneCam game hook (added with psxport a1c53d7c / Tomba2 84d9980) hand-retyped the scratchpad view-matrix decode and read R[2][2] from the HIGH halfword of the fifth CR word instead of the LOW one. RT33 is the low half of CR4 — Render::projActiveCr packs it back as exactly cr[4] = (uint32_t)R[2][2]. The corrupt view-Z row broke depth, near-plane clamp and per-object depth cull for the whole native projection path.

Fixed by collapsing the three hand-written copies of that decode into ONE Render::readSceneViewMatrix, sited beside projActiveCr, plus PSXPORT_SELFTEST=sceneview which round-trips pack->unpack through both functions and carries a negative control proving it discriminates the exact halfword bug.

Verified vs PSXPORT_ORACLE=1 at f900 (AUTO_SKIP) and f200/400/600/800/930 of replays/scene-transitions/hut-entry-door-freeze.pad — both hut fishermen render again. In-band evidence: producers census fieldEntityRender 5348 prims/37 frames -> 471980 prims/831 frames.

Also fixed in the same commit: the external/psxport gitlink was still a1c53d7c while the tree was built against 25dd7826, and game_hooks.cpp names fps60TemporalRotate which does not exist in a1c53d7c — a bare clone at the recorded pin did not compile.

Full write-up: docs/findings/camera.md (top entry).
