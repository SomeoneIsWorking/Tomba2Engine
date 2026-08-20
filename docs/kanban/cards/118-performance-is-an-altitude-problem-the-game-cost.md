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
