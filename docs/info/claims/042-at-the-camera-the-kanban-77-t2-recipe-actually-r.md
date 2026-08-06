---
id: C042
kind: claim
status: holds
created: 2026-08-06
tags: render
depends: game/render/fx_backdrop_plane.cpp
---

## Claim

At the camera the kanban #77 T2 recipe actually reaches (1F8000D2/D6/DA = 10382,-4344,9608 in area 14), the guest submits and OT-links EVERY quad the port draws for the waterfall backdrop — 0 of 80 dropped by either of FUN_80110CA4's own gates. No cull belongs at this producer at this framing.

## Evidence

The guest's GTE-FLAG gate (divide overflow SZ3<H/2, SX2/SY2 saturation past +-1024, IR saturation) and its OT-key gate (AVSZ4 -> +-0x50/0x78 bias -> grid-A near-snap -> log compression -> grid A drops k<4, grid B drops k outside [4,0x7FF]) reconstructed term-by-term inside the native producer from the same inputs it projects with, counting only: '[gteflag] f3940 quads=80 drawn=25+7 GTE-FLAG-error would drop 0 (divOverflow=0 sxSat=0 sySat=0 irSat=0); OT-KEY gate would drop 0'. NEGATIVE CONTROL same run earlier frames: f3043 drop 15/32, f3192 drop 28/38 — the gate can report a positive.

## What would falsify it

a capture at the USER's actual camera (20161,-1923,8268), which this recipe cannot reach — see instrument I047
