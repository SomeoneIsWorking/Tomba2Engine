---
id: 47
title: pc_skip: entering 'House on the Point' corrupts state (music stops, camera unfollows, interior vibrates)
status: todo
labels: [bug, pc-skip]
created: 2026-07-23
updated: 2026-07-23
---

USER 2026-07-23, reported from their LIVE windowed game. Entering the 'House on the Point' interior corrupts the game state. NOT a freeze or crash — the game keeps running (frame advances ~30fps, no recomp-MISS, no abort). The corruption is BEHAVIOURAL / state, multiple symptoms at once:
  - the BGM/music STOPS on entry;
  - the house INTERIOR GEOMETRY VIBRATES (per-frame jitter);
  - on going back OUTSIDE, the CAMERA STOPS FOLLOWING Tomba.
This cluster (music + camera + geometry all wrong together) points at a single upstream cause — a corrupted scene/area-load or a state block clobbered on the interior transition — rather than three independent render bugs. RE the House-on-the-Point interior LOAD / scene-transition path and find what state it corrupts.

REPRO (cut from the user's live session with padrec): replays/bugs/house-on-the-point.pad, 4134 frames from boot, hand-played route entering the house. Replay headless: PSXPORT_PAD_REPLAY=replays/bugs/house-on-the-point.pad ./scratch/bin/tomba2_port (see replays/README.md).

VERIFICATION METHOD — this is a STATE bug, do NOT crash-check: a headless replay exits 0 while the state is wrong. Verify by observing the actual symptoms: (a) camera-follow — probe the camera-target/follow latch and whether it tracks Tomba's world pos after exit; (b) music — the BGM sequencer state (0x801054B0 open-seq / 0x80104C28 playmask, PSXPORT_DEBUG=seq) going silent on entry; (c) interior vibration — capture consecutive frames of the interior and diff for per-vertex jitter. Compare against a CORRECT house entry if one can be reached, or against the oracle (PSXPORT_GATE=1 + the same replay) to see whether the corruption is exec-state (present on the oracle too = a real state bug) or pc_render-only.
FIRST TRIAGE STEP: run the replay under PSXPORT_GATE=1 (recomp_path, the oracle) and see if the symptoms reproduce there. If yes, it is a genuine game-state corruption in the transition logic (highest priority, a pc_faithful/Job-#1-class bug); if the symptoms are pc_render-only, it is a render/producer issue.

**2026-07-23:** 2026-07-23 USER CORRECTION: this is a PC_SKIP-EXCLUSIVE bug, NOT a render bug. It reproduces under mPcSkip=true (the default ./run.sh config, which the user's live game runs) and NOT on the faithful path. So it is a Job #1-class fork defect: the House-on-the-Point interior LOAD has a pc_skip shortcut (game->mPcSkip ? load_in_one_step() : load_in_multi_step_faithfully()) that collapses a multi-step init and gets the END-STATE wrong — dropping whatever the faithful branch sets up: the BGM restart, the camera-follow latch, and the interior geometry/transform init (hence music stops, camera stops following on exit, interior vibrates — all three are state the collapsed init failed to establish).
TRIAGE (corrected — the fork is pc_skip, not GATE): reproduce with the default (mPcSkip=true) on replays/bugs/house-on-the-point.pad, then run the SAME replay with the FAITHFUL branch forced (PSXPORT_SKIP=0 / the pc_skip=false path — see docs on how run.sh selects it) and confirm the symptoms vanish. That isolates it to the fork. Then SBS-full (both cores mPcSkip=false is the byte gate; but here the DIVERGENCE is skip-vs-faithful, so use MODE=skip positive-observable compare, or diff the state the faithful branch sets that the skip branch omits). RE the House-on-the-Point interior init fork and add the missing steps to load_in_one_step (or route the affected sub-inits through the faithful path). Reference the pc_skip fork rules: no inline if-else (feedback_no_pc_skip_inline_blocks), collapse must bump tick counters (feedback_pc_skip_frame_counter_bump), never route pc_skip=0 to substrate. The camera-follow latch and BGM sequencer state are the two most probeable end-states to check first.
