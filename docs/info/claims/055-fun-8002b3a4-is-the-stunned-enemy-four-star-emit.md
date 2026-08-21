---
id: C055
kind: claim
status: holds
created: 2026-08-21
tags: render,tomba2,effects
depends: game/render/fx_sprite.cpp#Render::fxSpriteEmit, game/render/mesh_quads.h#MeshQuads::composeScaled, psxport.pin
reconfirmed: 2026-08-21 11:48:52
verified_at: 2026-08-21 11:48:52
---

## Claim

FUN_8002B3A4 is the stunned-enemy four-star emitter, and its native transform must preserve RotMatrix Q12 units then apply the three 0x800A1CD4 bytes shifted left by two before camera composition.

## Evidence

replays/bugs/stun-stars.pad real Circle hit; generated/shard_4.c:2498-2725 transform chain; live renderpath native f2799 versus renderpath psx f2800; docs/findings/effects.md

Repin verification against definitive psxport `692b9b20e3d4a6194452522060fd2657c2235f40`: after the preliminary 9f framework forced a full recompilation-substrate re-emission, the final pin received another clean Clang 22.1.8 rebuild. A bounded SBS true-oracle run reached f2802 cleanly. Native A and `PURE-ORACLE(interp+softGPU)` B both visibly retain the four-star cluster at f2799. The native producer reports `drawn=4/4`; its four centres span 28.38 by 7.89 pixels. All six A/B captures and all 61 ring telemetry lines are byte-identical to the retained 9f run. Evidence: `scratch/logs/repin_692_stun.log`, `scratch/screenshots/repin_692_stun_f2799_{A,B}.ppm`.

## What would falsify it

if the replay no longer spawns a visible 0x8002B3A4 node with a stunned owner, the generated guest body no longer performs RotMatrix plus Math::matColScale from 0x800A1CD4, or the native and software-oracle star clusters cease to agree

## Re-confirmed 2026-08-21 11:48:52

Post-landing clean Clang 22.1.8 build and true-oracle replay on psxport 692b9b20 exited at f2802; all 61 ring telemetry lines and three A/B frame pairs match retained evidence, with drawn=4/4 at 28.38x7.89 pixels.
