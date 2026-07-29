---
id: 68
title: Area-4 ambient effect FUN_8013B118 is gated off in every reachable state (phase 1, fade 4096)
status: todo
labels: [render,verification]
created: 2026-07-29
updated: 2026-07-29
---

All three branches are off where we can get to: story phase 0x800E7EAA = 1 (branch A needs >= 44; tail needs 2/3/4) and node 0x800EDC90 fade +0x58 = 4096 skips the mesh panels. So the fn draws nothing and any port is unverifiable. Needs a scene with phase 2/3/4 or >= 44. Two further blockers: the 342-point field (ov_a04_func_8013AD90, 218 lines of raw GP0 tile emit) has no analogue in game/render/ and wants its own card; and the mesh IR0 cue uses Rng::next(), which WRITES the guest seed 0x80105EE8 and so is forbidden to a read-only producer. Blocks portmap fx-area4-ambient-13b118. Spec: docs/re/render-targets-static-re.md.
