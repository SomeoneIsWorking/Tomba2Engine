---
id: 9
title: Synchronous loads skip loading-only screens entirely
status: done
labels: [loading, enhancement]
created: 2026-07-22
updated: 2026-08-14
---

FUN_8007FD54 is only the guest wait-loop indicator. Its sole owner is FUN_80044BD4, so hiding the
text was the wrong layer: it left the loading state alive. The product now completes FUN_80044BD4
through one synchronous task owner. Host work and the spawned task finish before return, so no wait
tick or loading-screen service exists to render. The generated multi-frame body remains available
only through the explicit oracle path.

Evidence: `scratch/logs/synchronous_wait_single_path.log` reaches free roam with authored flag-2 and
flag-3 completions, each reporting `wait_ticks=0 loading_services=0`; the framework
`test_synchronous_task_wait` exercises the same completion seam for flags 1, 2, and 3.
