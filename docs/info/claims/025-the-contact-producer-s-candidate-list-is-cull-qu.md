---
id: C025
kind: claim
status: holds
created: 2026-07-30
tags: 
---

## Claim

The contact producer's candidate list is cull queue A (0x1F80013C ptr / 0x1F800144 count, cap 24), fed only by objects whose class byte obj[+0x0C] is 2 or 9 via Cull::enqueueByClass; our natives reproduce the push faithfully

## Evidence

line-by-line read of game/render/cull.cpp:96-247 against guest 0x8007712C/8007703C/80077E7C; consumer read width confirmed mem_r8 vs mem_w16 publisher, benign at cap 24

## What would falsify it

a measurement showing queue A receiving an object whose obj[+0x0C] is not 2 or 9, or a Cull native edit that changes the push
