---
id: C004
kind: claim
status: holds
created: 2026-07-23
tags: softlock
---

## Claim

kanban #60 SEQUENCE softlock still reproduces on current main and is NOT kanban #50's flat-task wait truncation

## Evidence

PSXPORT_DEBUG=sched over the whole replay never emits 'caught a GAME substate yield' (scratch/logs/sched60.log); Tomba does not move under 240 frames of held left/right at f7100 (scratch/logs/r60b.log, screenshots a_right240/a_left240.png)

## What would falsify it

2026-10-23
