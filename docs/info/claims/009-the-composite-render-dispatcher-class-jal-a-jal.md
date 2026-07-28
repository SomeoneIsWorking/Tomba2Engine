---
id: C009
kind: claim
status: holds
created: 2026-07-28
tags: render
---

## Claim

The composite render dispatcher class ({ jal A; jal B; } as a node render fn) has exactly ONE member: 0x80033080, and it is handled.

## Evidence

Scanned every distinct stored code pointer in a field dump (3231 MAIN.EXE + the overlay window) for a frame-opening jal-only function. 5 hits: 0x8001DB38 (area-data load), 0x80033080 (the impact burst), 0x80042690 (sound command), 0x801231B0 (item gate), and 0x8013ED08/0x8013EF58 (effect-mesh controllers wired the same day). Only one is render. 2026-07-28.

## What would falsify it

if the recompiled binary changes, or if a render fn turns out to dispatch through a jalr rather than a jal (the scan rejects jalr outright)
