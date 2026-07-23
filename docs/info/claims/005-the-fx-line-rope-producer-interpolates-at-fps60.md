---
id: C005
kind: claim
status: holds
created: 2026-07-23
tags: 
---

## Claim

the fx_line rope producer interpolates at fps60

## Evidence

debug ropeline over 6 logic frames: fps60 OFF gives x = 214.7, 222.1, 231.1, 236.3, 241.2, 244.8; fps60 ON gives those same values plus 212.3, 219.8, 228.9, 234.4, 240.0, each strictly between its real neighbours (5/5) - a non-lerping producer prints each value twice

## What would falsify it

a ropeline dump where the two presents of a logic frame print identical positions
