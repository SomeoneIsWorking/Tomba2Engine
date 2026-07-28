---
id: C015
kind: claim
status: holds
created: 2026-07-28
tags: render
---

## Claim

Render::altSpriteEmit's scale numerator was read UNSIGNED while the guest reads it signed — a latent ~65536x scale bug on the fxRotSpriteTailRender path

## Evidence

ov_a01_gen_8012D9E8 body spans generated/ov_a01_shard_1.c:16073-16412 and line 16382 inside it reads (int16_t)mem_r16(r17+16) == node+0x70 sign-extended; shipped fx_sprite.cpp:583 read (uint16_t) into a uint32_t AltSprite::numerX. Found 2026-07-28 by the static-RE verify pass, confirmed by hand against the shard. Fixed to mem_r16s + int32_t fields. Never observed in play because fxRotSpriteTailRender is COLD across the 15-replay library; regression frames (area 21 field, walk-dust-puff) byte-identical after the fix.

## What would falsify it

a scene where node+0x70 is provably always non-negative would make it unobservable but not wrong; a gen body for 8012D9E8 reading lhu would falsify it outright
