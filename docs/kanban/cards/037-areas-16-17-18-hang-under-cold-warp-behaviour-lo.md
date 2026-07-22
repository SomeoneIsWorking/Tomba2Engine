---
id: 37
title: areas 16/17/18 hang under cold warp — behaviour loop in gen_func_80040558
status: todo
labels: [bug]
created: 2026-07-22
updated: 2026-07-22
---

Cold warp ('newgame; skip 60; warp N; skip 40') into areas 16, 17 and 18 hangs: watchdog STUCK, no frame presented, backtrace Engine::fieldFrame -> ObjectList::walkAll -> BehaviorDispatch::dispatchObj -> gen_func_80040558 -> ... -> Core::mem_w32. Not a rec_dispatch miss. Still hangs with PSXPORT_WATCHDOG=60.

PRE-EXISTING, proven: the 2026-07-22 area-table recompiler fix (#24) changed the emitted function set of ov_a0g/ov_a0h/ov_a0i by exactly ZERO functions (module-by-module diff against scratch/genold/), so this predates it.

Probably the same root as kanban #36(A) — the settled-warp recipe ('newgame; skip 3000; warp 16; skip 600') completes cleanly and renders, so the hang may be a symptom of the cold warp leaving the area byte / object set inconsistent rather than an area-16..18 defect. Check #36 first. Logs: scratch/logs/areasweep/s16.log, w16.log (long watchdog), z16.log (settled, clean).
