---
id: C006
kind: claim
status: holds
created: 2026-08-27
tags: tomba1,widescreen,projection
depends: game/core/sync_native.cpp#platformHlePlan
---

## Claim

SCUS_942.36 publishes its resident projection through libgte SetGeomOffset at 0x80063A34 and SetGeomScreen at 0x80063A54: the initialized projection is 320x224 centered at (160,112) with H=544, and 0x8002D784 is the only later resident caller of SetGeomScreen.

## Evidence

Generated instruction bodies and Ghidra decompilation agree: 0x80016AF4 calls SetGeomOffset(0xA0,0x70) and SetGeomScreen(0x220), while display-environment construction at 0x80016C4C passes 0x140 by 0xE0 rectangles. Ghidra's reference database reports 2/2 callers of 0x80063A54, at 0x80016B0C inside 0x80016AF4 and 0x8002D89C inside 0x8002D784; 0x80063A34 has exactly the initialization caller.

## What would falsify it

Raw disassembly identifies another resident SetGeomOffset/SetGeomScreen entry or caller, the observed running title publishes different arguments, or display-environment construction does not produce 320x224 rectangles.
