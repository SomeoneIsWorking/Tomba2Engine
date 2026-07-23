---
id: C006
kind: claim
status: holds
created: 2026-07-23
tags: 
---

## Claim

Script op12 (SceneEvents::applyFlagOp) writes the scene-flag table at 0x800BF870+idx+324, matching gen_func_80042448; scene-flag readers (op04/op06) and writers now share one definition in game/scene/scene_flags.h

## Evidence

cw watch caught the pre-fix store at 0x800BF9CB vs op04's 0x800BF9EB; post-fix the #60 rendezvous completes (meet == 3) and replays/bugs/sequence-softlock-2.pad no longer deadlocks

## What would falsify it

a scene-flag store landing outside 0x800BF9B4..0x800BFA00 for a valid slot, or any new file hard-coding its own copy of the table base
