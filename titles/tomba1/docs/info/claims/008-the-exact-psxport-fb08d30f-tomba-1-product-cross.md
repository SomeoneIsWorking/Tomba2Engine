---
id: C008
kind: claim
status: holds
created: 2026-08-27
tags: product,movie,dma
depends: game/core/sync_native.cpp#dmaCallbackOverride
---

## Claim

The exact psxport fb08d30f Tomba! 1 product crosses the former nine-INT1 movie stall and presents a decoded Whoopee Camp frame

## Evidence

PID 3376435: scratch/logs/tomba1-direct-dma-fb08d30f.console.log dispatches DMA3 callback 0x80066D80, continues genuine CdReady/INT1 delivery through at least LBA 58739, and chains MDEC-out callback 0x8001F4D4; scratch/screenshots/present_600.ppm was captured in that run and independently inspected as a clean visible Whoopee Camp logo frame while present_540.ppm is black.

## What would falsify it

A provenance-matched replay fails to dispatch 0x80066D80, fails before LBA 57839, or present 600 is shown not to originate from PID 3376435.
