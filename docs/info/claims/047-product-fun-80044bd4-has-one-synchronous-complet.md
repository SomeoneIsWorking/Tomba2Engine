---
id: C047
kind: claim
status: holds
created: 2026-08-14
tags: scheduler,loading
depends: game/core/main.cpp#main, game/core/engine.cpp#Engine::submode1Case0Native, game/scene/demo.cpp#Demo::s0Native, game/scene/sop.cpp#Sop::fieldMode
---

## Claim

Product FUN_80044BD4 has one synchronous completion path and PSXPORT_PC_SKIP no longer selects a product cadence

## Evidence

scratch/logs/synchronous_wait_single_path.log and scratch/logs/synchronous_wait_retired_pc_skip_final.log; framework test_synchronous_task_wait

## What would falsify it

a current product run observes a nonzero synchronous wait tick/loading service, or setting PSXPORT_PC_SKIP changes the reached wait path or timeline
