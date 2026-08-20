---
id: 118
title: Performance is an ALTITUDE problem: the game costs 0.00ms, our machinery costs 4.42ms
status: todo
labels: []
created: 2026-08-20
updated: 2026-08-20
---

USER, 2026-08-20: "How fast do you think a PSX era game should run on this hardware?" The right question, and the phase profiler answers it in one line.

MEASURED, 1,100 frames of ingame-options-page.pad, unpaced, clang, QUIET machine (Ryzen 7 5700X), PSXPORT_DEBUG=perf:

    frame 4.42 ms = pre 0.02 | tick-LOGIC 0.00 | audio 0.30 | PRESENT-cpu 2.42 | SCHED-LOGIC 1.28 | post 0.40

    phase                                ms     % of frame
    PRESENT-cpu (our renderer)         2.42        54.8
    SCHED-LOGIC (task scheduler)       1.28        29.0
    post                               0.40         9.0
    audio                              0.30         6.8
    pre                                0.02         0.5
    tick-LOGIC (THE GAME)              0.00         0.0

THE GAME IS FREE. All of Tomba!2 — AI, physics, scripts, the whole recompiled MIPS substrate — measures 0.00 ms. 100% of the frame is the port's own machinery.

WHAT THAT MEANS FOR THE TARGET. If the game costs nothing, the ceiling is set entirely by our overhead. A renderer submitting a few hundred textured quads at 320x240 and a cooperative scheduler with three task slots should together cost on the order of 0.1-0.3 ms:
    at 0.30 ms/frame  ->  3,300 fps   (111x realtime)
    at 0.15 ms/frame  ->  6,700 fps   (222x realtime)
We measure 226 fps, 7.5x realtime. So this port is roughly 15-30x slower than it should be, and none of that gap is the game.

FOR SCALE: the PSX gave this game 1.13M CPU cycles per frame at 33.8 MHz. We spend ~18M cycles per frame on a 4 GHz superscalar core — 16x the CYCLE COUNT on a core doing far more per cycle. An INTERPRETING emulator does better than that, and we are a native recompilation.

WHY MY OWN PERF WORK (#117) WAS THE WRONG ALTITUDE. The three fixes there are real and verified — 30.9%, pixel-identical — but they optimise INSIDE the 55%, and never asked why PRESENT-cpu is 2.42 ms at all. 30% off the wrong number leaves the number wrong.

THE TWO ITEMS THAT ACTUALLY MATTER, in size order:

1. PRESENT-cpu 2.42 ms. The host profile says ~29% of the frame is render-queue ORDERING, and what that code does is sample an 8x8 interior grid PER PAIR OF FACES, testing point-in-triangle in scalar CPU float, to decide draw order. That is CPU rasterisation used to answer an ordering question. A depth buffer answers it for free, in the GPU, which is the whole reason the port has per-vertex depth. The right question is not "make the contest faster" but "why does a native renderer with a real depth buffer need a CPU face-ordering contest at all". Note kanban #29 / #11: the contest exists for coincident and same-OT-bucket faces the depth buffer genuinely cannot order — but that is a SMALL subset, and it currently costs a full pairwise scan of every keyed face in every node, every frame.

2. SCHED-LOGIC 1.28 ms — 29% of the frame, for a cooperative scheduler with THREE task slots. Nothing about three slots should cost a millisecond. Not yet investigated at all; it has never appeared in a profile because nobody profiled until today.

DO NOT START by micro-optimising either. Both numbers are 10-100x larger than the work they represent, which means the cost is structural, and a structural cost is found by asking what the code is doing per frame that it should be doing once — the same shape as the producer-DB cache the USER pointed out.

REPRO:
  PSXPORT_NOAUDIO=1 PSXPORT_NO_FMV=1 PSXPORT_NOPACE=1 PSXPORT_NATIVE_FRAMES=1100 \
  PSXPORT_PAD_REPLAY=replays/bugs/ingame-options-page.pad PSXPORT_DEBUG=perf ./scratch/bin/tomba2_port
CHECK `uptime` FIRST — a loaded machine makes every number here meaningless (see #117).

**2026-08-20:** 2026-08-20 — CORRECTION, SAME DAY: "THE GAME COSTS 0.00 ms" IS WRONG. I misread a legacy counter. The game costs 1.28 ms.

WHAT I DID: read `tick-LOGIC 0.00` off the perf line and reported that all of Tomba!2 measures nothing.

WHAT PHASE 0 ACTUALLY MEASURES: game_tomba2.cpp:84-86 brackets ONE call — rec_dispatch(c, 0x800788AC), described as "real per-frame state update (still-PSX leaf)". That was the whole per-frame update when the port ran the guest's own loop. It is not any more: the work moved to PcScheduler, so phase 0 now brackets a path that no longer carries it and honestly reports 0.00 for what it still measures.

WHERE THE GAME ACTUALLY IS: phase 3, and native_boot.cpp:157 says so in a comment — "SCHED-LOGIC = the cooperative scheduler step (the real per-frame GAME logic)", wrapping pcSched.step(). That is 1.28 ms.

    phase                                ms     % of frame   what it is
    PRESENT-cpu                        2.42        54.8      our renderer (world build + VRAM upload + VK submit)
    SCHED-LOGIC                        1.28        29.0      THE GAME (recompiled substrate + native game code)
    post                               0.40         9.0
    audio                              0.30         6.8
    pre                                0.02         0.5
    LOGIC                              0.00         0.0      A DEAD COUNTER — brackets a path the work left

THIS IS EXACTLY THE FAILURE THE PROJECT RULES NAME: a diagnostic that can only print nothing. Phase 0 cannot distinguish "this is free" from "the work moved and I am not measuring it any more", and it prints the same 0.00 for both. It has been printing that on every perf run since the scheduler took over, and today it fooled me into telling the USER the game was free.

THE REVISED PICTURE, and it is less dramatic but still bad:
  * The PSX gave this game ~1.13M CPU cycles per frame at 33.8 MHz. We spend 1.28 ms on a 4 GHz core = ~5.1M cycles — about 4.5x the CYCLE COUNT the original used, from statically recompiled MIPS running on a superscalar core that should beat the original per cycle, not lose to it.
  * The renderer at 2.42 ms for a few hundred textured quads at 320x240 remains the clearest outlier and is still the biggest single item.

A REALISTIC TARGET, stated as arithmetic rather than ambition: renderer to ~0.2 ms and substrate to ~0.3 ms gives ~0.9 ms/frame -> ~1,100 fps, ~37x realtime. That is a 5x improvement available, not the 15-30x I claimed. The claim of 3,000-6,000 fps rested on the game being free and is withdrawn.

FIRST ACTION FOR WHOEVER PICKS THIS UP: fix phase 0 — either point it at where the work went or delete it. A counter that reads 0.00 for "not measured" will mislead the next person exactly as it misled me. That is a workflow defect and it outranks the optimisation.

**2026-08-20:** 2026-08-20 — PRESENTER FIXED, the way the card said to fix it. psxport 26974cd4.

USER: "fix the presenter, it makes no sense to have intensive contest CPU thing". Right — and the fix was not to make the sampling faster but to stop sampling.

ord is AFFINE per triangle, so (ord_far - ord_near) is affine over the region both faces cover, that region is convex, and an affine function's maximum over a convex polygon is at a VERTEX. Clip one triangle by the other, evaluate at the survivors. Exact, and a handful of evaluations instead of 128.

    wall clock    5.044 -> 4.071 s      -19.3%
    PRESENT-cpu   2.42  -> 1.75 ms      -27.7%
    frame         4.42  -> 3.29 ms      226 -> 304 fps

IT CHANGES WHAT IS DRAWN, deliberately. 118 of 262 faces snap where the grid snapped 109; 192,639 inversions found where the grid found 152,304. The grid was missing REAL inversions — the "sub-sample sliver" residual the file documented was not theoretical. On screen that is 19 pixels of 691,200 at f1090 (0.003%), so it removes latent depth-fighting rather than restyling anything.

TWO BUGS ON THE WAY, both caught by the SNAP COUNT and neither by the hermetic oracle test:
  * boundary points: clipping admits points ON an edge, so mesh-adjacent faces survived as a zero-area sliver and reported an inversion for every adjacent pair — 221 of 262 snapped. Fixed by requiring the shared region to have AREA.
  * (in a separating-axis pre-filter, since removed) separating on <= treated edge-touching faces as disjoint and silently DROPPED inversions — 98 of 262.
The oracle test passed through both because it does not exercise edge-touching pairs. THE REAL SCENE'S SNAP COUNT IS THE GATE for anything touching this code; record it before and after, every time.

SESSION TOTAL ON THIS CARD'S TWO ITEMS:
    item                       before      after
    PRESENT-cpu (renderer)     2.42 ms     1.75 ms
    GAME-LOGIC (the game)      1.28 ms     1.01 ms   (unchanged code — cache effects)
    frame                      4.42 ms     3.29 ms
    fps                        226         304

STILL OPEN, and item 2 of this card is untouched: SCHED-LOGIC / GAME-LOGIC at ~1.0 ms for a three-slot cooperative scheduler plus the recompiled substrate. That is now the largest remaining item after the renderer, and nobody has looked at it. The renderer at 1.75 ms is still ~10x what submitting a few hundred quads should cost, so it is not finished either — but the cheap structural win there has been taken.

**2026-08-20:** 2026-08-20 — MORE PROFILING, per the USER. The quarter of the frame that resolved to nothing now has a name, and one confident guess about it was wrong.

THE PROFILER NAMES SHARED OBJECTS NOW (psxport ad4cea5b). hostprof captures /proc/self/maps at dump time — that correspondence exists only inside the live process — and prof_hot.py attributes out-of-executable samples to the module, plus a symbol within it where one is exported WITH A SIZE.

    [libc.so.6]              16.96%
    [libstdc++.so.6.0.35]     3.34%
    [libvulkan_radeon.so]     2.73%

NOT the GPU driver, which was the obvious guess. It is memory work, and it is the single largest entry in the profile — larger than any function we wrote.

A WRONG NAME, CAUGHT: the first version of the symbol lookup fell back to "nearest preceding export" and reported 15.6% of the frame in libc's `_dl_mcount_wrapper` — a profiling hook that cannot be hot. Names inside a shared object are now claimed only when the sample is INSIDE the symbol's own extent; libc's hot paths are IFUNC-resolved and frequently are not where the exported name sits. Same failure the tool already guards against for `data_start`.

WHAT IS KNOWN ABOUT THE libc TIME: 62% of its samples fall inside a 0x26-BYTE SPAN — one tight loop, the signature of a bulk copy, at a rate consistent with write-combined memory.

WHAT IT IS NOT — and this is the useful negative. The obvious suspect was render_geom's 1 MB whole-VRAM snapshot memcpy into a WC transfer buffer, unconditional, every frame. Gating it on the existing s_vram_writes counter SKIPPED IT ON 50.0% OF CALLS (1,096 of 2,192) and changed the frame by nothing:
    4.071 -> 4.112 s wall, PRESENT-cpu 1.75 -> 1.74 ms — both inside noise.
So halving that copy is free, which means the copy is not what costs. The gate was reverted rather than kept: by the same standard that removed a 1.6% separating-axis filter earlier today, a 0% change does not earn its state.

NEXT STEP, precisely: install glibc debug symbols (`dnf debuginfo-install glibc`) and re-run prof_hot.py, which will then resolve that span by name instead of by extent. Everything else here is guesswork until it has a name — I have now been wrong about it once.

CURRENT FRAME, for the record:
    PRESENT-cpu   1.74 ms
    GAME-LOGIC    1.03 ms
    audio         0.30 ms
    post          0.20 ms
    frame         3.30 ms   ->  303 fps  (10.1x realtime)
Started the session at 226 fps / 4.42 ms.
