---
id: C048
kind: claim
status: holds
created: 2026-08-20
tags: render
depends: psxport.pin
reconfirmed: 2026-08-20 22:47:24
verified_at: 2026-08-20 22:47:24
---

## Claim

Native equal-key opaque world ties reproduce PSX AddPrim LIFO order on the deterministic HUT replay

## Evidence

hut-entry-door-freeze.pad f1200: Native pixel (140,80) changed srgb(32,32,16) to srgb(24,64,64), exactly GTE; 2,657/76,800 Native pixels changed; second view unobstructed; 5/5 forced interpolation pairs 0/76,800

## What would falsify it

the replay packet order changes, an equal-key AddPrim fixture no longer maps to strictly ordered D32 depths, or Native pixel (140,80) no longer matches GTE

## Re-confirmed 2026-08-20 22:47:24

Confirmed with the same tested framework content committed as psxport 81cb8e05, followed by an explicit
Clang reconfigure/build and a successful psxport pin/provenance check. The focused 17-face AddPrim
property and painter tests pass; deterministic HUT f1200 remains exact at the GTE probe with 5/5
interpolation pairs identical.
