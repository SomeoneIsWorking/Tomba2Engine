---
id: 10
title: Verify the ~58 untested beh_* rebuilds by faithful-body A/B (finds bugs of the confirmed class)
status: todo
labels: [verification]
created: 2026-07-22
updated: 2026-07-22
---

**2026-07-22:** Measured 2026-07-22: game/ai has 61 beh_*.cpp handlers, only 3 with an // ORACLE: marker, so ~58 have NO equivalence test against the guest. port_check cannot check them - they are rebuilds and legitimately differ in store order. The working method, demonstrated on FUN_8007DC38 (dialog driver): port_gen the byte-faithful body (PASS by construction), swap the behavior-table entry to call it, run a replay with PSXPORT_PAD_DUMP_AT, diff the 2MB dumps, swap back. Zero diff = equivalent on that path; a diff is a candidate bug of the class that produced kanban #8 and #1. Prioritise the rebuilds that actually run on replays/bugs/bucket-softlock.pad. CAUTION: PAD_DUMP_AT writes a fixed filename - rename each dump immediately or the baseline is overwritten.
