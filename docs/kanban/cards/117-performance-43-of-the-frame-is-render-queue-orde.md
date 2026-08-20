---
id: 117
title: Performance: ~43% of the frame is render-queue ORDERING, ~9% is a diagnostic
status: todo
labels: []
created: 2026-08-20
updated: 2026-08-20
---

USER, 2026-08-20: "the performance is horrible". MEASURED rather than guessed, and the answer is not where I would have guessed.

THE INSTRUMENT DID NOT WORK, and that came first. hostprof.cpp (PSXPORT_PROF) had a header, a knob, a sample format and a named companion tool — and hostprof_init() was CALLED FROM NOWHERE, so the exit audit said "UNKNOWN knob PSXPORT_PROF was set for this whole run and NOTHING ever read it". Its reader, tools/prof_hot.py, did not exist either (tools/prof_report.py is a DIFFERENT profiler's reader, working in guest addresses, and it crashes on a hardcoded missing path). Both fixed: psxport e3ecffa9.

MEASUREMENT: 2,000 frames of replays/bugs/ingame-item-menu.pad, PSXPORT_RENDER_PATH=psx, PSXPORT_NOPACE=1, clang build. 11,469 samples, 0 dropped, 79.9% resolved.

      %tot   symbol
     18.81   rq_ord_at
     16.18   rq_face_extent
      6.66   OtAttr::resolveClaimedFrame
      6.39   rq_faces_in_contest
      3.46   EObjXform::projectFlags
      3.34   SPU_UpdateFromCDC
      2.48   OtAttr::trackStoreSlow
      1.69   GpuVkState::tex_emit
      1.32   RenderQueue::resolveKeyOrderFaces
     20.07   <unresolved — outside every text symbol>

TWO FINDINGS:

1. ~43% OF THE FRAME IS RENDER-QUEUE ORDERING. rq_ord_at + rq_face_extent + rq_faces_in_contest + resolveKeyOrderFaces = 18.81 + 16.18 + 6.39 + 1.32 = 42.70%. Not rasterization, not the recompiled game code, not the GTE — the SORT. rq_faces_in_contest being a pairwise predicate alongside rq_face_extent suggests an O(n^2) contest over queue items, which would explain why it dominates a scene with only a few hundred prims. NOT YET CONFIRMED as O(n^2) — that is the next measurement (prim count vs time, two scenes of different complexity), and it must be measured before anyone optimises.

2. ~9% IS A DIAGNOSTIC. OtAttr::resolveClaimedFrame 6.66% + OtAttr::trackStoreSlow 2.48% = 9.14%. CLAUDE.md exempts diagnostics from the no-stamping rule and says they stay — correctly, they answer questions the picture cannot. But nine percent of every frame on a NORMAL run is a real cost, and 'trackStoreSlow' names itself. Worth checking whether it can be gated to runs that ask for attribution, without losing the always-on property that makes it useful.

WHAT THE 20% UNRESOLVED IS NOT: it is not a hot function. An earlier ad-hoc resolution credited it to `data_start` (the nearest preceding symbol), which reads as a real 20% entry and is really 'fell outside every text symbol'. prof_hot.py now reports it as its own line. Reducing it — the likely candidates are PLT stubs and vendored objects — would sharpen the ranking but does not change the relative order of what IS resolved.

CAVEAT ON SCOPE: one scene, one render path. The item menu is 368 prims of mostly 2D. A 3D field scene will have a different mix and MUST be profiled before generalising — the START page (f1090 of ingame-options-page.pad, 971 prims of real 3D) is the obvious second sample.

REPRO:
  PSXPORT_NOAUDIO=1 PSXPORT_NO_FMV=1 PSXPORT_NOPACE=1 PSXPORT_RENDER_PATH=psx \
  PSXPORT_NATIVE_FRAMES=2000 PSXPORT_PAD_REPLAY=replays/bugs/ingame-item-menu.pad \
  PSXPORT_PROF=1 ./scratch/bin/tomba2_port
  python3 external/psxport/tools/prof_hot.py scratch/raw/prof_host.txt scratch/bin/tomba2_port
