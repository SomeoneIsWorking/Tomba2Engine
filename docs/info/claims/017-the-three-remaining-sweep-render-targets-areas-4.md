---
id: C017
kind: claim
status: falsified
created: 2026-07-29
tags: render
falsified_on: 2026-07-29
---

## Claim

The three remaining sweep render targets (areas 4, 8, 14) are all reached by the pc_render walk and can be pixel-verified; the area-21 one cannot

## Evidence

warp N + skip 600 with PSXPORT_DEBUG=nofx on 2026-07-29 names 0x8013B118 (a4), 0x80116904 (a8) and 0x80110CA4 (a14) in each area's census, alongside only terrain 0x8002AB5C and margin 0x8013CDD4. FUN_8010C1D8 (a21) by contrast returns immediately: its A0L phase gate 0x800BFA55 reads 1 and it requires >= 4

## What would falsify it

a producer added for one of the three that emits zero times in that area, which would mean the census named a node the display walk does not actually dispatch

## FALSIFIED 2026-07-29

TOO STRONG on its 'can be pixel-verified' half. Being named in the nofx census proves only that the WALK REACHES a live node carrying the fn — it says nothing about whether the fn's OWN internal gates let it draw. FUN_8013B118 (area 4) is named in the census yet draws NOTHING reachable: story phase 0x800E7EAA = 1 in two independent dumps (branch A needs >= 44; the tail needs 2/3/4) and node 0x800EDC90 fade +0x58 = 4096 skips the mesh panels. Area 14 was fine (ported, 27904 px). CORRECTED FORM: the census proves DISPATCH reachability, not CONTENT reachability; content must be checked per target before porting.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
