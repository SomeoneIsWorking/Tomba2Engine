---
id: 2
title: Direct-runtime ReadS enters the finite stock-read burst
status: resolved
symptom: A continuous movie ReadS is misclassified as a finite stock read and reaches the 65,536-sector guard
tags: tomba1,disc,streaming,direct-runtime
state_items: S004
created: 2026-08-27
updated: 2026-08-27
---

# Direct-runtime ReadS enters the finite stock-read burst

- State: resolved
- Affects: S004
- Owner: shared psxport CD command policy plus the Tomba! 1 runtime fact seam

## Symptom

The 2026-08-27 real-disc product run advanced past the generated `OPTSUB00` overlay, issued ReadS
(`0x1B`), and then reported that the stock read did not terminate after 65,536 sectors. No frame was
presented afterward, so the watchdog aborted the process after its 20-second progress deadline.

## Root cause

Tomba! 1 is a direct runtime, so `core.cfg` is null. The shared ReadN/ReadS branch correctly labels
the operation a continuous stream and arms `Cd::stream_active`, but then calls the finite
`cd_drive_stock_read` burst when `!core.cfg || !core.cfg->cdReadStock`. The null direct-runtime arm
therefore chooses the opposite behavior from the branch's documented contract: an unbounded movie
stream is driven as though it were a finite file read and can only stop at the safety bound.

The title now publishes its measured ready-callback pointer slot (`0x80095FF0`) and registers a
59.940 Hz call-coherent stream field. The ordinary native field boundary cancels duplicate timer
turns. That owner cannot run while ReadS is monopolized by the synchronous finite burst.

## Resolution

The shared `PlatformHlePlan` now has typed `cdReadAddress` and `cdReadSyncAddress` facts. Tomba! 1
publishes its measured leaves there, so the framework owns finite `CdRead` synchronously and the
ReadN/ReadS command owner selects the continuous stream without inferring behavior from a null
legacy `GameConfig`. The finite burst remains intact for its actual legacy consumers.

## Evidence

- `scratch/logs/tomba1-stream-product.log`: correct CHD, overlay read to `0x800E7388`, ReadS command,
  then the 65,536-sector guard.
- `scratch/logs/tomba1-stream-product.console.log`: watchdog stack and abort after the stream halt.
- `scratch/logs/tomba1-typed-read-product.log`: the same real disc and overlay route pass ReadS
  without entering the 65,536-sector burst; execution continues into the stream consumer.
- `tests/test_stream_field_turn.cpp`: shipping host-turn callback is a no-op both without a callback
  layout and without an active stream, publishes both typed finite-read leaves, and keeps them out
  of the generic title binding table.
