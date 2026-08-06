---
id: C041
kind: claim
status: holds
created: 2026-08-06
tags: render
depends: game/render/fx_backdrop_plane.cpp
---

## Claim

Area 14's 'wall of water' (kanban #77 T2) is drawn by Render::fxBackdropPlaneRender (guest FUN_80110CA4, A0E overlay, node 0x800ED960 on HEADS[1]) — NOT by the scene table (fieldEntityRender / dbgnode FFFF0002).

## Evidence

Break-first A/B in an isolated clone, both legs' build exit status checked, legs distinguished by in-band channel counts not pixels: suppressing fieldEntityRender = 6001/76800 px, bbox y[117..227] (docks+barrels only, wall untouched); suppressing fxBackdropPlaneRender's drawWorldQuad = 63112/76800 px, bbox x[0..319] y[12..227], upper half goes black. tools/primat_filter.py at probes (160,40)/(40,20)/(280,90) frame 3940: 2 genuine covers each, all dbgnode=800ED960, vs 26-28 rejected degenerate hits. Shots scratch/shots/t2-producer/.

## What would falsify it

a capture where the upper-half wall survives fxBackdropPlaneRender being suppressed
