---
id: C051
kind: claim
status: holds
created: 2026-08-21
tags: render,oracle
depends: external/psxport/runtime/recomp/shaders_gpu/psx_uv.glsl, external/psxport/runtime/recomp/gpu_vk_texture_phase_selftest.cpp#gpu_vk_run_texture_phase_selftest, game/render/field_hud.cpp#fieldHudItemRing
---

## Claim

The Tomba 2 health wheel native packet now rasterizes pixel-exactly against the interpreter software-GPU pane because the shared host textured path evaluates affine UV at PSX integer native pixels; the producer packet remains byte-identical to guest GP0.

## Evidence

Guest GP0 and native queue both carry FT4 xy (39,47) (55,47) (39,31) (55,31), uv (56,15) (72,15) (56,31) (72,31), CLUT (496,203), tpage 0006. The shipping texture-phase gate moved from 2/5 at 1x to 20/20 at 1x/3x opaque/semi, and true-SBS palette-mask comparison is 0/340 at both f560 and f561.

## What would falsify it

If the captured guest/native packet pair, psx_uv.glsl integer-pixel reconstruction, one of its shader consumers, the shipping phase selftest, or the final f560/f561 pane captures changes.
