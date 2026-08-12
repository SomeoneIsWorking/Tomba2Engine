---
id: 87
title: Producer census guest rows all report 'frames 1 (f3..f3)' — gpu.s_frame counts presents
status: backlog
labels: [bug,render]
created: 2026-08-12
updated: 2026-08-12
---

2026-08-12: every guest row in the run-end census reports frames 1 (f3..f3) regardless of how long it drew. gpu.s_frame counts PRESENTS, which do not advance the same way on the guest leg. Same bug class already fixed for the span reset. Makes the row's lifetime field useless for triage.
