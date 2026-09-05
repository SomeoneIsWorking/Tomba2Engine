---
id: C050
kind: claim
status: holds
created: 2026-08-21
tags: oracle,tooling
depends: external/psxport/runtime/psx/native_boot.cpp#dc_boot_init, external/psxport/runtime/psx/sync_overrides.cpp#PlatformHle::register_, external/psxport/runtime/psx/sbs.cpp, game/core/game_hooks.cpp#tomba_devWarp
---

## Claim

SBS constructs per-Game CD and PlatformHle services and the shared game-owned cold warp reaches area 4 on both the native and pure-interpreter legs

## Evidence

Clang-built SBS: boot RAM+scratchpad identical, both player-controllable at f246, PSXPORT_SBS_WARP cold-warps both at f300, pane captures at f560/f650/f800/f900, clean exit f930 with no libcd timeout or historical guest-entry miss; standalone area-4 warp ran through f927

## What would falsify it

a fresh two-leg boot reports libcd retry/timeouts or boot-state divergence, or either leg fails to reach the same destination through GameHooks::devWarp
