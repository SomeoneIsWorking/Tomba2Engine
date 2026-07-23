---
id: C005
kind: claim
status: holds
created: 2026-07-23
tags: softlock
---

## Claim

kanban #60's softlock is a two-actor script rendezvous DEADLOCK on scene-flag slot 55 (0x800BF9EB), not a scheduler/render/beh_* defect

## Evidence

cut-mode 0x1F800137 stuck at 1 forever; objects 800FC9E0 and 800FC7D0 both parked in script op 0x04 phase 1, writing 2/awaiting 1 and writing 1/awaiting 3 with the byte stuck at 2 (scratch/logs/{flagw,obj2,partner}.log); ruled out by force-gen: all 65 beh_* handlers, areaSeasidePerframe, node3_router, area_event_dispatch, ScriptInterp::init, outerTransitionGate, mode0Action/WalkHandler; identical under PSXPORT_RENDER_PSX=1

## What would falsify it

2026-10-23
