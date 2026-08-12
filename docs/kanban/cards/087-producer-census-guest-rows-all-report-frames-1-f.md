---
id: 87
title: Producer census guest rows all report 'frames 1 (f3..f3)' — gpu.s_frame counts presents
status: done
labels: [bug, render]
created: 2026-08-12
updated: 2026-08-12
---

2026-08-12: every guest row in the run-end census reports frames 1 (f3..f3) regardless of how long it drew. gpu.s_frame counts PRESENTS, which do not advance the same way on the guest leg. Same bug class already fixed for the span reset. Makes the row's lifetime field useless for triage.

**2026-08-12:** 2026-08-12 FIXED (psxport 63c5f537). Root cause: both census feed sites stamped rows with GpuState::s_frame, which counts PRESENTS. Same root cause as the already-fixed OtAttr span-reset bug one layer down — fixing the reset without fixing the row stamp left the identical defect in the field a human reads. Now one shared definition, census_frame.h -> Timing::logicFrame. Verified: 0x8010C26C frames 34 (f59..f92), 0x80078CA8 frames 64 (f30..f95); ranges line up with the native leg for the same scene.
