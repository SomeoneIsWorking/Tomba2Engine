---
id: 9
title: Remove pure loading-only screens
status: doing
labels: [loading, enhancement]
created: 2026-07-22
updated: 2026-08-31
---

FUN_8007FD54 is only the guest wait-loop indicator for an area-load task. Its sole owner is
FUN_80044BD4, so hiding the text was the wrong layer: it left the loading state alive. The product
now completes FUN_80044BD4 through one synchronous task owner. Host work and the spawned task finish
before return, so no wait tick or loading-screen service exists to render. The generated multi-frame
body remains available only through the explicit oracle path.

That completed game-load path is distinct from the front-end's static `Loading.....` card.
`docs/tomba2-clips.md` identifies the latter as the StrPlayer control coroutine at resume PC
`0x800452C0` while the engine latch at `0x800BE258` is zero. `docs/tomba2-waits.md` shows that its
short CD load is already complete before roughly 1,800 zero-CD dwell frames. Remove that pure
loading-only card through the StrPlayer command's real completion/cleanup route into the attract
sequence, not through a timer, drive-idle pacing bypass, or a write to coroutine/command state.

Evidence: `scratch/logs/synchronous_wait_single_path.log` reaches free roam with authored flag-2 and
flag-3 completions, each reporting `wait_ticks=0 loading_services=0`; the framework
`test_synchronous_task_wait` exercises the same completion seam for flags 1, 2, and 3. The separate
front-end card discriminator is documented in `docs/tomba2-clips.md`.
