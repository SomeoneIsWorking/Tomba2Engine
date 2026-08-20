---
id: 110
title: Make psx_render a TRUE oracle: wire beetle's vendored GPU rasterizer (not PsyCross)
status: done
labels: [render, oracle]
created: 2026-08-20
updated: 2026-08-21
---

USER 2026-08-20: "Try to setup a true oracle like maybe https://github.com/OpenDriver2/PsyCross
works better ... Because there are other oracle issues too ... PsyCross or beetle"

## Resolution

Beetle's software GPU is wired as an independent GP0/GP1 rasterizer tee. PsyCross was rejected for
this role because its packet parser ultimately renders through OpenGL rather than producing PSX
15-bit VRAM; it remains useful only as a readable SDK/PGXP reference.

The tee is deliberately not named a whole-machine oracle. Both rasterizers consume the port's same
command stream, so it can diagnose GPU semantics but cannot catch wrong game state or packet
generation. Card #119 owns the separate true interpreter leg.

## Trust gate and repairs

The census is inside Beetle where command boundaries exist. It reports accepted/dropped words,
dispatched primitives, starvation, unknown/known-no-op commands, FIFO depth, and mirrored GPUREAD
results. The 1-pixel shift control has produced the required non-matching answer.

Repairs made while calibrating it:

- native VRAM uploads are mirrored into Beetle;
- draw time is granted before `GPU_WriteCB`, which drains commands synchronously without a synthetic
  scanout clock;
- `GPU_StartFrame` runs only at a real guest-frame boundary;
- every GP0(C0) readback word is drained on both implementations;
- GP0 0x00 and 0x01 are classified as legitimate no-ops;
- quad continuations and frame/primitive denominators are counted at their real boundaries.

Area-4 validation accepted 5,417 words with zero dropped/starved/queued/unknown commands, reconciled
969/969 primitives, and mirrored 19,712 GPUREAD words with zero mismatches. The wheel crop differed
by five pixels, each within one 5-bit output step. Those figures validate the rasterizer tee only;
they do not override the real-game references or the production-shader gate on card #22.
