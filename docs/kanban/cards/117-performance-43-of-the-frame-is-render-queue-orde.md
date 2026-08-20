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

**2026-08-20:** 2026-08-20 — CORRECTION: THE HEADLINE FINDING ON THIS CARD IS WRONG. Do not act on it. I published "~43% of the frame is render-queue ORDERING" and it cannot be true.

HOW IT WAS CAUGHT — by checking the instrument against a denominator the code already keeps. resolveKeyOrderFaces prints a per-frame census on `PSXPORT_DEBUG=keyord`. Over the SAME 2,000-frame run that produced the profile:

    6,000 keyord lines, and EVERY ONE says "0 keyed faces of 0 queued prims — nothing to contest"

So the contest loop did ZERO work for the entire run. And rq_face_extent / rq_ord_at are reachable from NOWHERE ELSE — call-site check: rq_face_extent only at render_queue.cpp:1822/1823 and rq_ord_at only at :1848/:1851, both inside rq_faces_in_contest, which is called only at :1935 inside that loop. Functions that are never called cannot be 35% of the frame.

WHAT IS ACTUALLY WRONG: the SYMBOL ATTRIBUTION, not the sampling. The sampler is fine — 11,469 samples, 0 dropped, and ITIMER_PROF is CPU time. The mapping from host PC to name is what lies: these are `static` functions in an -O3 build, so they are inlined or folded, and their nm symbol addresses no longer bound the code that actually sits there. prof_hot.py credits a sample to the nearest preceding symbol within range, which is exactly wrong when the symbol table and the code layout have diverged.

SO EVERY NAME IN THAT TABLE IS SUSPECT, not only the top two. OtAttr at 9.1% and SPU_UpdateFromCDC at 3.3% are extern, non-static symbols and are more likely to be real — but "more likely" is not measured, and I am not going to repeat the mistake of reporting a ranking this tool produced as if it were established.

THE INSTRUMENT IS MARKED DISTRUSTED at symbol granularity. It is trustworthy for: total sample count, dropped count, and the unresolved fraction. It is NOT trustworthy for: which function.

WHAT WOULD FIX IT, in order of preference:
  1. perf record + perf report, which symbolises through inlining using DWARF rather than guessing from nm ranges. Needs the build to keep frame pointers or use --call-graph dwarf.
  2. Failing that, profile a build with -fno-inline for the render queue TU only — but note that changes the thing being measured, so it can locate a hot region and must not be quoted as a percentage of the real build.
  3. Cross-check any candidate against a counter the code already keeps, the way keyord just falsified this one. That check is cheap and it is what should have run BEFORE the card was written.

WHAT REMAINS TRUE AND UNAFFECTED: the profiler was dead (hostprof_init called from nowhere) and its reader did not exist; both are genuinely fixed (psxport e3ecffa9), and the run does now produce 11,469 real samples. The repair stands; the interpretation does not.

THE ACTUAL PERFORMANCE QUESTION IS THEREFORE STILL OPEN, with no measured answer yet.

**2026-08-20:** 2026-08-20 — THE CORRECTION ABOVE WAS ITSELF WRONG. The original finding STANDS, and is now confirmed by a counter rather than by the profiler alone. Read this note, not the one before it.

WHAT I GOT WRONG IN THE CORRECTION: I compared two runs that used DIFFERENT RENDER PATHS and treated the disagreement as proof the profiler was lying.
    the profile run   had NO PSXPORT_RENDER_PATH  -> NATIVE path
    the keyord run    had PSXPORT_RENDER_PATH=psx -> psx software rasteriser
On the psx path the render queue is empty by construction (the software rasteriser draws directly), so "0 keyed faces of 0 queued prims" was the correct answer FOR THAT PATH and said nothing whatever about the native one. Both measurements were right; putting them side by side was not.

CONFIRMED ON THE NATIVE PATH, same replay, PSXPORT_DEBUG=keyord, 1,200 frames — 6,010 of 7,751 keyord lines report real work, and a representative frame reads:

    f1201 resolveKeyOrder: 109/262 keyed faces snapped (14772 pair tests, 911 queued prims)

14,772 PAIR TESTS IN ONE FRAME, for 262 keyed faces out of 911 queued prims. Each pair test is rq_faces_in_contest, which calls rq_face_extent TWICE and then samples an interior grid calling rq_ord_at per sample. That is the 18.81% + 16.18% + 6.39% the profiler reported, and the profiler was right: the samples fall INSIDE the sized nm ranges of those symbols (checked explicitly — rq_ord_at spans 0xe3dcc0+350 and the hot PCs are at +0x1f, +0x7a, +0xa6, +0xab, +0x110, +0x114).

SO THE MECHANISM IS ESTABLISHED, not hypothesised: the contest is quadratic in the size of a single node's face group. 262 faces in one group is ~14.8k tests even WITH the existing witness/early-break optimisation, because that optimisation only helps faces that get snapped (109 of 262 here) — the 153 that never find a witness each scan their whole group.

WHERE THAT LEAVES THE 20% UNRESOLVED: identified. Those samples are at 0x7f4b6a75xxxx — SHARED LIBRARY addresses, outside the executable's text entirely. prof_hot.py correctly refuses to name them. They are libc/SDL/driver work, not a missing hot function of ours.

WHAT IS STILL NOT MEASURED: whether OtAttr's 9.1% is real (it is an extern symbol so the attribution is more robust, but it has not been cross-checked against a counter the way this was), and the whole picture on a 3D field scene rather than this menu.

THE INSTRUMENT IS NOT DISTRUSTED. The previous note marked it so on a false premise; that is withdrawn. prof_hot.py's symbol attribution was verified correct here by two independent means — sized-range containment, and an in-code counter that agrees with it.

**2026-08-20:** 2026-08-20 — FIRST FIX LANDED: 18.1% faster. psxport 3416594f.

rq_faces_in_contest recomputed BOTH faces' extents on every call. An extent depends only on the face, but the contest is asked about PAIRS — so one face's extent was recomputed once per partner it was tested against. At f1201 that is 29,544 computations of 262 distinct values in a single frame, which is exactly why rq_face_extent showed as 16.18%.

The caller has the whole group in hand, so it now computes each face's extent once per frame. The rule moved into rq_faces_in_contest_ext (takes precomputed extents); the public two-argument rq_faces_in_contest computes them and delegates, so there is ONE implementation and the brute-force oracle test keeps calling the same entry point.

    before   7.922  7.980  7.939 s   mean 7.947
    after    6.508  6.574  6.455 s   mean 6.512     -> 18.1% faster
Every after-run beat every before-run. tests/test_render_queue_keyorder.cpp (brute-force oracle) passes, which is the gate that matters for a change claiming to be pure memoisation. Suite 62/63, the one failure being the pre-existing game_iface.h cap.

WHAT THIS DOES NOT DO, stated so nobody reads the card as closed: the QUADRATIC IS UNTOUCHED. Still 14,772 pair tests for 262 faces, and rq_ord_at's interior-grid sampling is still the largest single entry at 18.81%. Two follow-ups, in order of expected value:

  1. EXTENT-OVERLAP PRUNE BEFORE THE GRID. The cheap rejects (bbox overlap, depth-range overlap) already exist INSIDE the contest, but every pair still pays a function call and both extent lookups to reach them. With extents now precomputed and contiguous, the caller can reject non-overlapping pairs without calling the contest at all — and on a mesh most pairs in a group do not overlap. This is the change that should actually cut the 18.81%.
  2. THE GROUP ITSELF. 262 faces in one node's group is the real driver. A spatial index (grid or interval list over the precomputed extents) turns "scan the whole group" into "scan the neighbours", which is the only way the 153 faces that never find a witness stop costing a full scan each.

Do 1 before 2: it is small, it composes with the memoisation just landed, and it will show whether the grid sampling or the pair enumeration is the remaining cost.

**2026-08-20:** 2026-08-20 — SECOND FIX: 22.6% FASTER OVERALL. psxport 71b4689b.

rq_ord_at recomputed, for EVERY sample of the 8x8 interior grid, values that depend only on the face — six vertex fetches and the triangle determinant. The contest asks two faces for their ord at 64 points, so that work repeated 64x per face per PAIR, with 14,772 pairs in a frame. That is why it was the single largest entry at 18.81%.

rq_face_setup() now computes it once per face per frame (alongside the extent from the first fix) and rq_ord_at_setup() samples from it. The arithmetic is character-for-character the old per-sample code, INCLUDING keeping the division by `den` rather than folding in a reciprocal — the contest's answer is a comparison, so a boundary case that rounds the other way flips a snap decision.

    original            7.922  7.980  7.939 s   mean 7.947
    after extent memo   6.508  6.574  6.455 s   mean 6.512     -18.1%
    after setup memo    6.151  6.132  6.182 s   mean 6.155     -22.6% from original

TWO GATES, NOT ONE, because "pure memoisation" is a claim and claims get tested:
  * tests/test_render_queue_keyorder.cpp — the rule against a brute-force oracle. Passes.
  * THE PRESENTED FRAME at f1120 of the item menu is PIXEL-IDENTICAL to the pre-optimisation capture: 0 of 691,200 differing. An unchanged picture is the strongest available statement that nothing about the ordering decision moved.

A NOTE ON MY OWN CORRECTION EARLIER ON THIS CARD: I predicted the next win would be an extent-overlap prune in the caller. That was WRONG and I checked before writing it — the cheap bbox/depth rejects already run inside the contest before the grid, so a caller-side prune would duplicate work already done. The reason rq_ord_at stayed hot is that many pairs genuinely PASS those rejects and pay the full 64-sample grid. Recorded so the next person does not implement the prune I talked myself out of.

WHAT IS STILL NOT DONE: the quadratic. 14,772 pair tests for 262 faces is unchanged — both fixes made each test cheaper, neither reduced the count. The remaining lever is a spatial index over the precomputed extents so a face scans its neighbours rather than its whole group; that is what would help the 153 of 262 faces that never find a witness and so scan everything. Worth doing only if a 3D field scene shows the same shape — this measurement is still one menu scene.
