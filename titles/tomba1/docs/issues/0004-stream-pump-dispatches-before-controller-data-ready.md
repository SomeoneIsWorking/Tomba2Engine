---
id: 4
title: Stream pump dispatches before controller data-ready
status: resolved
symptom: The first movie callback is invoked with response reason 0 before the controller has an INT1 data-ready response
state_items: S004
tags: tomba1,disc,streaming,controller
created: 2026-08-27
updated: 2026-08-27
---

# Stream pump dispatches before controller data-ready

- State: resolved
- Affects: S004
- Owner: shared psxport continuous-stream pump

## Symptom

The native `CdReady` owner clears the guest-VSync violation, but the real product still stops
presenting after black frame 540. The active task no longer yields while the main thread waits in
`Tomba1FrameDriver::runTaskSlot`/`Coro::resume`.

## Root cause

The shared stream pump forces its first callback at elapsed time zero and increments
`stream_delivered` before dispatch. The controller schedules first-sector availability separately
in deterministic guest time. On Tomba! 1's first forced callback, no controller response exists and
the first sector is still pending. `StCdInterrupt` only exits early for reason 5 because a real
`CdReadyCallback` is invoked only when a response is ready; reason 0 therefore enters its BFRD/DMA
consumer without a sector. Guest ticks then make INT1 current, but the consumer and controller FIFO
are already out of phase.

## Proper fix

The shared pump must service the deterministic controller clock and dispatch/count a callback only
when the controller has a current data-ready response. That decision belongs beside the shared
`stream_delivered` pacing counter. Returning a fabricated disk error from the title adapter,
decrementing the counter afterward, or special-casing Tomba! 1 would hide the ownership error.

## Evidence

- `scratch/logs/tomba1-cdready-reason.console.log`: exact isolated sequence is
  `reason=0 queue=0->0 read=1 first=1 data=0/0 lba=57830`, followed by
  `reason=1 queue=0->1 read=1 first=0 data=0/0 lba=57830`.
- `scratch/logs/tomba1-cdready-isolated.console.log`: the product passes the former guest-VSync
  violation, captures an entirely black 960x720 present 540, then the watchdog reports the task's
  non-yield.

## Resolution

The shared pump now services the deterministic controller clock and dispatches/counts only a
current INT1 data-ready response. Isolated execution produces no reason-0 callback; repeated genuine
INT1 responses advance the controller from LBA 57830 through 57838. Issue 0005 owns the later,
distinct scheduling halt.
