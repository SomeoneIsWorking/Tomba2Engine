---
id: C017
kind: claim
status: holds
created: 2026-07-29
tags: render
---

## Claim

The three remaining sweep render targets (areas 4, 8, 14) are all reached by the pc_render walk and can be pixel-verified; the area-21 one cannot

## Evidence

warp N + skip 600 with PSXPORT_DEBUG=nofx on 2026-07-29 names 0x8013B118 (a4), 0x80116904 (a8) and 0x80110CA4 (a14) in each area's census, alongside only terrain 0x8002AB5C and margin 0x8013CDD4. FUN_8010C1D8 (a21) by contrast returns immediately: its A0L phase gate 0x800BFA55 reads 1 and it requires >= 4

## What would falsify it

a producer added for one of the three that emits zero times in that area, which would mean the census named a node the display walk does not actually dispatch
