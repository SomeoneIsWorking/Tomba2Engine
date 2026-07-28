---
id: C014
kind: claim
status: holds
created: 2026-07-28
tags: render
---

## Claim

Render::fxParticleFieldRender (FUN_8010C7F4, A0L/area 21) is a pixel-verified native producer, and Trig::vecLen ports its FUN_80078240 distance helper

## Evidence

area 21 warp + skip 600, ON vs producer-removed OFF leg = 2069 px differ at x[67..182] y[64..193] (soft grey mist against black sky, scratch/screenshots/field21/field_offVSon.png); 554 emissions drawn=32/64; 0x8010C7F4 gone from the nofx census leaving only terrain/margin/0x8010C1D8; emitAnimQuadRecords ir0/far defaults are identity, proven by a byte-identical walk-dust-puff.pad frame vs the pre-change binary

## What would falsify it

an SBS run showing either the producer or Trig::vecLen writing guest memory, or a frame where the field's logged screen extent is on-frame but ON and OFF match
