---
id: 3
title: Stream CdReady uses the guest VSync timeout clock
status: resolved
symptom: The movie stream callback reaches guest VSync at return address 0x80065728
tags: tomba1,disc,streaming,vsync
state_items: S004
created: 2026-08-27
updated: 2026-08-27
---

# Stream CdReady uses the guest VSync timeout clock

- State: resolved
- Affects: S004
- Owner: Tomba! 1 libcd synchronization adapter

## Symptom

After correcting the stream callback slot from `0x80095FEC` to `0x80095FF0`, the real disc reaches
the callback's `StCdInterrupt` body and immediately triggers the mandatory guest-VSync violation at
`0x80067C30`. The exact caller return address is `0x80065728`.

## Root cause

The linked `CdReady` wrapper at `0x800648E8` calls the controller-response owner at `0x800656F0`.
That routine seeds and polls its 960-field timeout using `VSync(-1)` before consuming the current CD
controller response. The port's native frame loop owns the field clock, so executing that guest
timeout query violates the title contract even when a data-ready response is already queued.

The earlier callback address was also wrong: `CdReadyCallback` at `0x80064920` writes `0x80095FF0`
and ReadS setup installs `StCdInterrupt` (`0x80066CA8`) through that setter. `0x80095FEC` belongs to
the other libcd callback and retained the generic `0x800646F4` event handler, which could never
publish a stream ring entry.

## Resolution

The title binds the measured `CdReady` wrapper to a native nonblocking controller-response adapter.
It services the deterministic controller clock, copies and acknowledges exactly the current
response, preserves the controller bank, and consumes the CD interrupt edge because the title host
turn directly invoked the ready callback in place of the guest IRQ path. An absent response returns
zero without waiting. Guest VSync remains fatal and unchanged.

## Evidence

- `scratch/logs/tomba1-ready-slot-product.console.log`: real disc reaches the corrected callback,
  then aborts at guest VSync with RA `0x80065728` and a stack through `0x800648F8`, `0x80067164`, and
  `0x80066CB8`.
- `tests/test_stream_field_turn.cpp`: the shipping native `CdReady` adapter consumes a synthetic INT1
  response and produces the opposite no-response result without waiting.
- `scratch/logs/tomba1-cdready-isolated.console.log`: isolated real product passes the former guest
  VSync violation and reaches present 540 before the next stream-consumption stall.
