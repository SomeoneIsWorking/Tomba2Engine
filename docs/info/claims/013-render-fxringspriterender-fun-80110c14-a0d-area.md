---
id: C013
kind: claim
status: holds
created: 2026-07-28
tags: render
---

## Claim

Render::fxRingSpriteRender (FUN_80110C14, A0D/area 13) is a pixel-verified native producer: 21 sprites per frame from one node

## Evidence

area 13 warp + skip 200, ON vs producer-removed OFF leg = 202 px differ at x[2..75] y[151..211]; ON 610 emissions drawn=21/21, OFF 0 emissions and nofx names 0x80110C14; both legs one source revision, distinct binary md5s, object-swap relink; side-by-side crop scratch/screenshots/ring13/ring_offVSon_zoom.png shows the blue sparkle arc present only in ON

## What would falsify it

a frame where the ring's logged screen extent is inside the viewport but ON and OFF still match, or an SBS run showing this producer writing guest memory
