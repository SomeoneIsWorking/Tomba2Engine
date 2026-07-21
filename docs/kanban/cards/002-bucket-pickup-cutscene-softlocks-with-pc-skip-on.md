---
id: 2
title: Bucket-pickup cutscene SOFTLOCKS with pc_skip ON (default config)
status: doing
labels: [bug, pc-skip]
created: 2026-07-17
updated: 2026-07-22
---

USER live 2026-07-15: picking up a bucket initiates a cutscene that SOFTLOCKS under the default config (pc_skip=true). Likely a collapsed-init/wait fork in the cutscene trigger where the pc_skip shortcut branch fails to advance (a wait that never completes, or a phase-gate counter not bumped — cf. the pc_skip frame-counter-bump rule). Repro: replay the bucket-pickup pad capture default config, watch for the cutscene hang after bucket pickup; compare pc_skip=false (SBS Core A / MODE=skip) which should progress. Blocks gameplay progress.

**2026-07-22:** While chasing this, built tools/width_audit.py (compares every literal guest address access in game/ against the widths the GUEST uses in the real instruction stream) and it found 5 real defects in Engine::fieldRun - the pc_skip path, which is where every one of the reported bugs lives. The worst two: 0x800BF870 is the area-id BYTE (144x lbu / 1x sb in the guest, never wider) but was read as mem_r32 and compared ==0x15, which can essentially never be true since the read also covers bf871..73 - so that state transition NEVER FIRED under pc_skip; and it was WRITTEN with mem_w32, zeroing three adjacent live bytes every time state 6 ran. Also three mem_r32(0x800E7FEE) where the guest uses lh. All fixed, build clean. Whether any of this addresses the bucket dialog softlock is UNVERIFIED - needs a live retest.
