---
id: 6
title: Linked libcd internal CdSync reaches guest VSync after movie playback
status: fixing
symptom: After visible movie progression, linked libcd calls VSync(-1) from return address 0x800654A4
state_items: S004
tags: tomba1,disc,streaming,vsync
created: 2026-08-27
updated: 2026-08-27
---

## Symptom

The exact psxport `fb08d30f` product run reaches visible decoded movie output and continues through
at least LBA 58739. It then aborts at the mandatory guest-VSync trap `0x80067C30`, with `a0=-1` and
return address `0x800654A4`. The stack includes the movie MDEC callback chain at `0x8001F50C` and
`0x8001F48C`. This is later than and independent of the resolved DMA3 callback loss in issue 0005.

## Binary-grounded cause

Executable disassembly shows that public `CdSync` at `0x800648C8` is a four-instruction wrapper that
forwards its arguments unchanged to internal linked-libcd owner `0x80065470`. The recorded title
runtime bound only the public wrapper, while linked libcd also calls `0x80065470` directly from its
asynchronous command-completion path. That direct call bypassed the native owner; the internal body
seeds its 960-field timeout with `VSync(-1)`, producing the observed return address `0x800654A4`.

## Preserved migration state

The in-flight title source binds measured internal entry `0x80065470` to the same native owner as the
public wrapper. That edit was compiled before Tomba! 1 was deprioritized but was never executed, so it
is not a verified fix. Tomba! 1 now waits behind the Tomba! 2 Lightrec migration. When it resumes, the
shipping dynarec product must cross the former `0x800654A4` violation through image-aware native
dispatch without weakening the fatal VSync trap. The deleted static product is not an available path
to close this issue.

## Evidence

- `scratch/logs/tomba1-direct-dma-fb08d30f.console.log`: visible movie progression followed by the
  fatal VSync call with `a0=-1`, `ra=0x800654A4`, and the recorded guest stack.
- Executable disassembly at `0x800648C8` and `0x80065470`: exact wrapper forwarding and internal timeout seed.
- `game/core/sync_native.cpp` and `tests/test_stream_field_turn.cpp`: compiled title-local binding
  and focused assertion awaiting execution/product proof.
