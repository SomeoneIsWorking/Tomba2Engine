---
id: 68
title: Area-4 ambient effect FUN_8013B118 is gated off in every reachable state (phase 1, fade 4096)
status: todo
labels: [render, verification]
created: 2026-07-29
updated: 2026-08-06
---

All three branches are off where we can get to: story phase 0x800E7EAA = 1 (branch A needs >= 44; tail needs 2/3/4) and node 0x800EDC90 fade +0x58 = 4096 skips the mesh panels. So the fn draws nothing and any port is unverifiable. Needs a scene with phase 2/3/4 or >= 44. Two further blockers: the 342-point field (ov_a04_func_8013AD90, 218 lines of raw GP0 tile emit) has no analogue in game/render/ and wants its own card; and the mesh IR0 cue uses Rng::next(), which WRITES the guest seed 0x80105EE8 and so is forbidden to a read-only producer. Blocks portmap fx-area4-ambient-13b118. Spec: docs/re/render-targets-static-re.md.

**2026-08-06:** 2026-08-06 (G10 survey) — one of the three blockers on this card is stale. The PRNG blocker ('the mesh IR0 cue uses Rng::next(), which WRITES the guest seed 0x80105EE8 and so is forbidden to a read-only producer') is ANSWERED: GuestRngMirror exists at game/render/guest_rng_mirror.{h,cpp}, is in cmake/tomba2_port.cmake:252, and is already referenced from fx_backdrop_plane.cpp. It is a per-logic-frame read-only snapshot of the seed with the guest's own LCG constants (0x41C64E6D/12345, draw = (seed>>16)&0x7FFF) that never writes guest memory, and its header records why a per-frame re-seed is the right answer rather than a compromise. Do not re-derive that design. The other two blockers stand and are real: (1) every branch is gated off in the only reachable area-4 state (story phase 0x800E7EAA = 1, node 0x800EDC90 fade +0x58 = 4096), so a port could not be pixel-verified; (2) the 342-point field in ov_a04_func_8013AD90 is a 218-line raw GP0 tile emitter with no analogue anywhere in game/render/ and wants its own card. See docs/unported-render-inventory.md R5.
