---
id: C008
kind: claim
status: holds
created: 2026-07-28
tags: render
---

## Claim

0x8013D454 is the water-jet effect controller, and pc_render drew none of it until the A00 scope wrappers landed (2026-07-28).

## Evidence

A/B on replays/bugs/walk-dust-puff.pad frames 460/470/480/490/510/520 with the four engine_set_override_a00 installs compiled out: 700-1367 px differ per frame in a moving ~30x40 bbox tracking the stream; f500 alone matches (between pulses). Leg proven by fxmesh ctrl=8013D454 counts, 68 vs 0. Commit df97314.

## What would falsify it

if the jet also renders with the four installs removed on any other replay, the attribution is wrong and the picture came from a different producer

**Narrowed 2026-07-28 (same day):** 0x8013D454 has TWO branches on (s16)node+0x60. The non-zero branch goes through the mesh writer 0x80027768 and is what the scope wrappers fixed — that is the stream measured in the A/B and the pixel evidence stands. The ZERO branch emits through FUN_800328EC (see C010) and still draws nothing. Read this claim as 'the jet is restored', NOT as '0x8013D454 is fully owned'.
