---
id: C046
kind: claim
status: holds
created: 2026-08-14
tags: scene,kanban-47
depends: generated/shard_3.c#gen_func_80055E28, generated/shard_3.c#gen_func_80063158, generated/shard_4.c#gen_func_80065A54, game/scene/scene_transition.cpp#SceneTransition::stepSwapWaiter
---

## Claim

House-on-the-Point sm[0x4c]==3 is the authored interior mode; the natural exit owner is input-facing gen_func_80055E28, not state-4 gen_func_80063158

## Evidence

docs/findings/scene.md 'CORRECTION — state 3 is the interior mode' plus shipping logs scratch/logs/house_exit_facing_short_current.log and scratch/logs/house_g5_writers_bt_current.log

## What would falsify it

a shipping-path capture with no exit input naturally changes sm[0x4c] away from 3, or an exit changes sm[0x4c] 3->2 without gen_func_80055E28 first writing G+0x147 to the door-facing value
