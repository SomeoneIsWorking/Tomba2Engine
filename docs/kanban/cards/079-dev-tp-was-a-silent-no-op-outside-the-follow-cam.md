---
id: 79
title: dev tp was a silent no-op outside the follow-camera mode — FIXED
status: done
labels: [tooling]
created: 2026-08-06
updated: 2026-08-06
---

MEASURED 2026-08-06 on tomba2_port at 741b756: 'tp X Y Z' fired in area 0 (1 [tp] line; Tomba X 3940->6020) and fired ZERO times in areas 13, 14 and 20 over 300 frames each, with his master position unchanged — while the REPL still printed 'tp camera -> (x,y,z)'. Cause: the pending teleport was consumed inside CutsceneCamera::trackXZ, which only runs in the follow-camera mode. Fix: Engine::devTeleportApply, called from Engine::frameUpdate — the one per-frame body native_step_frame runs on every exec path. Verified after: areas 13 and 14 land the master position on the requested triple exactly and hold it for 300 frames; the negative control is the same probe on the pre-fix binary (scratch/bin/blockcull/tomba2_port_head), which moved nothing. Area 20 still does not fire: a settled warp there lands in a state where no frame body runs at all (kanban #36/#37), a separate bug. Also note the tp is a POSITION REQUEST, not a collision suspend — in area 0 the ground/walkable step re-derived Y and Z after the write.
