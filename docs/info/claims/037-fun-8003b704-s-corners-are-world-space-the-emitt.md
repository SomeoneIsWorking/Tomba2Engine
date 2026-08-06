---
id: C037
kind: claim
status: holds
created: 2026-08-06
tags: render
depends: game/render/fx_beam.cpp#beamQuadRender
---

## Claim

FUN_8003B704's corners are WORLD SPACE — the emitter loads the pure camera (scratchpad 0x1F8000F8) into GTE CR0-7 itself via SetRotMatrix/SetTransMatrix immediately before building them, so the caller's CR state (perObjRenderDispatch or billboardCompose1) is irrelevant to it

## Evidence

ground truth generated/shard_0.c gen_func_8003B704: r16 = (8064<<16)+248 = 0x1F8000F8, then func_80084660(r16) and func_80084690(r16); those two are libgte SetRotMatrix/SetTransMatrix (already established in docs/findings/render.md 'FUN_80039F4C text-label renderer OWNED'). Corroborated by the port drawing correctly: the native producer projects the same corners with the native camera and the resulting quad lands where the guest's does — 84 px appear at f652 of replays/bugs/weapon-impact-bucket.pad inside the producer's own predicted bbox, none outside it.

## What would falsify it

a scene where the beam draws in the wrong PLACE under pc_render while psx_render puts it correctly — that would mean the corners are relative to something the pure camera does not resolve. Also falsified if a future recomp of gen_func_8003B704 no longer shows the 0x1F8000F8 SetRotMatrix/SetTransMatrix pair.
