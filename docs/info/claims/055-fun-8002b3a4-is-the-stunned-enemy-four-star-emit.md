---
id: C055
kind: claim
status: holds
created: 2026-08-21
tags: render,tomba2,effects
depends: game/render/fx_sprite.cpp#Render::fxSpriteEmit, game/render/mesh_quads.h#MeshQuads::composeScaled
---

## Claim

FUN_8002B3A4 is the stunned-enemy four-star emitter, and its native transform must preserve RotMatrix Q12 units then apply the three 0x800A1CD4 bytes shifted left by two before camera composition.

## Evidence

replays/bugs/stun-stars.pad real Circle hit; generated/shard_4.c:2498-2725 transform chain; live renderpath native f2799 versus renderpath psx f2800; docs/findings/effects.md

## What would falsify it

if the replay no longer spawns a visible 0x8002B3A4 node with a stunned owner, the generated guest body no longer performs RotMatrix plus Math::matColScale from 0x800A1CD4, or the native and software-oracle star clusters cease to agree
