---
id: 116
title: Beetle oracle violates GPU.sl_zero_reached — NDEBUG hides it in every normal build
status: done
labels: []
created: 2026-08-20
updated: 2026-08-20
---

FOUND WHILE MOVING THE BUILD TO CLANG, and only because that build was accidentally configured WITHOUT -DNDEBUG. In every normal (Release) build this assertion is compiled out, so the oracle has been violating a beetle invariant silently since it was wired.

    tomba2_port: vendor/beetle-psx/mednafen/psx/gpu.c:2058: int32_t GPU_Update(const int32_t):
                 Assertion `GPU.sl_zero_reached == false' failed.

REPRO (any compiler — this is NOT clang-specific; it is NDEBUG-specific):
    configure the game WITHOUT CMAKE_BUILD_TYPE=Release, then
    PSXPORT_RENDER_PATH=psx PSXPORT_GPU_BEETLE=1 PSXPORT_NATIVE_FRAMES=30 ./scratch/bin/tomba2_port
It fires within the first ~30 frames, on the psx path, as soon as the oracle is on.

WHAT IT MEANS. gpu.c's scanline walk sets sl_zero_reached=true when it reaches scanline 0 and asserts it was false on the way in — i.e. it expects exactly ONE scanline-0 crossing per frame, cleared by the frame boundary that GPU_StartFrame/scanout provides. runtime/psx/gpu_beetle.cpp deliberately does NOT model scanout: it calls GPU_StartFrame ONCE at init and then drives GPU_Update with a synthetic monotonic timestamp purely to drain the command FIFO. So beetle's display state machine crosses scanline 0 repeatedly with nothing ever clearing the flag.

DO NOT read this as 'the oracle's results are wrong'. The scanout half is not what the oracle reads — it reads VRAM directly — and the VRAM answers have been independently corroborated: two of four measured screens are PIXEL-IDENTICAL to beetle (#112/#113 notes), the feed census reconciles exactly (368=368, 971=971, 70=70, 5=5), and the 1px positive control scores 1,692 PASS / 0 FAIL. An invariant violated in a subsystem we neither drive nor read has produced no measured error.

BUT IT IS STILL A REAL DEFECT, for two reasons:
  1. Driving a state machine outside its contract is exactly how a subtly wrong answer arrives later — the next beetle change that makes the rasteriser consult display state would silently corrupt the oracle, and the assert that would have caught it is compiled out.
  2. An assert firing in a vendored dependency is a signal, and NDEBUG turning it into silence is the same 'diagnostic that can print nothing' failure this project keeps writing rules about.

THE FIX, not yet attempted: give the adapter a real per-frame boundary — call GPU_StartFrame (or clear sl_zero_reached) once per guest frame instead of once at init, so the flag is cleared exactly when beetle expects. gpu_beetle_frame_report already runs at exactly that boundary, so the hook point exists.

VERIFY IT AFTER FIXING by keeping a build WITH asserts live: the whole value here is that an assert-enabled build catches contract violations a Release build cannot. Worth a CI/dev build config that does not define NDEBUG.

**2026-08-20 — FIXED AT THE ACTUAL CAUSE.** Calling `GPU_StartFrame` once per guest frame was necessary
but not sufficient. The first attempted fix still asserted because the adapter's synthetic `GPU_Update`
clock could cross scanline zero multiple times during ONE large native texture upload. That clock existed
only to drain the FIFO, but `GPU_WriteCB` already calls `ProcessFIFO` synchronously after
`psxport_gpu_grant_drawtime()`. The adapter now feeds commands without advancing Beetle's CPU/scanout clock
and calls `GPU_StartFrame` only at the real guest boundary. An assert-enabled Clang build completed the
30-frame reproducer and the full area-4 health run without the assertion. The same run also proved the
separate GP0(C0) read-drain repair: 19,712 GPUREAD words, 0 mismatches, 0 dropped/starved/queued input.
