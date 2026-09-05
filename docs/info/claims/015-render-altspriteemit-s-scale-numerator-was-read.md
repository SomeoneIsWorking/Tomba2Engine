---
id: C015
kind: claim
status: holds
created: 2026-07-28
tags: render
reconfirmed: 2026-08-06
verified_at: 2026-08-06
depends: game/render/fx_sprite.cpp#altSpriteEmit
---

## Claim

Render::altSpriteEmit's scale numerator was read UNSIGNED while the guest reads it signed — a latent ~65536x scale bug on the fxRotSpriteTailRender path

## Evidence

overlay guest 0x8012D9E8 body spans authenticated executable/overlay evidence and line 16382 inside it reads (int16_t)mem_r16(r17+16) == node+0x70 sign-extended; shipped fx_sprite.cpp:583 read (uint16_t) into a uint32_t AltSprite::numerX. Found 2026-07-28 by the binary-analysis verify pass, confirmed by hand against the shard. Fixed to mem_r16s + int32_t fields. Never observed in play because fxRotSpriteTailRender is COLD across the 15-replay library; regression frames (area 21 field, walk-dust-puff) byte-identical after the fix.

## What would falsify it

a scene where node+0x70 is provably always non-negative would make it unobservable but not wrong; a guest-visible behavior for 8012D9E8 reading lhu would falsify it outright

## Re-confirmed 2026-08-06

RE-VERIFIED 2026-08-06, statically, against BOTH sides. GUEST: overlay guest 0x8012D9E8 (authenticated executable/overlay evidence, 356 lines) reads the scale numerator at line 16478 as `(uint32_t)(int16_t)c->mem_r16((c->r[17] + (uint32_t)16))` — sign-extended, i.e. lh. NATIVE: game/render/fx_sprite.cpp:610 reads `c->mem_r16s(node + kRotTailScale)` with kRotTailScale=0x70 (line 260); the fix is still in place. NEGATIVE CONTROL for the signedness discriminator, in that same guest-visible behavior: 4 sign-extended vs 14 zero-extended halfword reads, so the check demonstrably produces BOTH answers and is not reading 'signed' off everything. NOTE ON THE ORIGINAL EVIDENCE: it cited ov_a01_shard_1.c:16382, which now holds unrelated code — authenticated executable/overlay evidence is rebuilt from the operator's own disc, so a authenticated executable/overlay evidence LINE NUMBER is not a stable citation. Re-located by scanning for the gen symbol instead. NOT re-verified: no runtime observation; fxRotSpriteTailRender remains cold across the replay library, exactly as the claim already says.
