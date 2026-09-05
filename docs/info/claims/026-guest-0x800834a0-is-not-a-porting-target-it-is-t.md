---
id: C026
kind: claim
status: holds
created: 2026-07-30
tags: port
reconfirmed: 2026-08-06
verified_at: 2026-08-06
depends: game/core/game_config.cpp
---

## Claim

Guest 0x800834A0 is NOT a porting target: it is the libgpu GPU-DMA-completion timeout ARM, already owned by PlatformHle as gpu_timeout_arm(), and its guest body must never run in this port.

## Evidence

Guest body (guest 0x800834A0, authenticated executable/overlay evidence) is 12 instructions: call guest 0x80085900(-1) = libetc VSync(-1) 'read the current VSync counter', add 240 (a ~4-second deadline at 60Hz), store to 0x800A5ADC, store 0 to 0x800A5AE0. PlatformHle installs gpu_timeout_arm on it from GameConfig::hle.gpuTimeoutArm=0x800834A0 (game/core/game_config.cpp:148) with gpuTimeoutDeadlineVar=0x800A5ADC and gpuTimeoutFlagVar=0x800A5AE0 — the same two globals the guest body writes — and arms 0x7FFFFFFF instead, because the native VK GPU runs the OT-DMA synchronously so the timeout must never fire. Porting the guest body faithfully would be BOTH a double-install AND a hang: 0x80085900 is registered as vsyncTrap and aborts with a guest backtrace on any caller in any mode (external/psxport/runtime/psx/sync_overrides.cpp:115). Live proof on this tree: headless run (newgame; run 600) with PSXPORT_DEBUG=recdep,plat-hle logs '18 hardware-sync primitive(s) installed' and 'top substrate dispatch targets ... 0x800834A0 : 2276   <-- ALREADY OWNED by PlatformHle (not a porting target)' — scratch/logs/recdep_800834A0.log:83.

## What would falsify it

if the port ever stops running the GPU/OT DMA synchronously (a real async GPU queue), the far-future deadline stops being correct and the real VSync-based arm would have to come back — at which point this address becomes a genuine port target

## Re-confirmed 2026-08-06

RE-VERIFIED 2026-08-06, statically, at every line the claim rests on; the two commits that flagged it stale (79d420e camera, abf3cf9 tap deletion) touch neither. game/core/game_config.cpp now has .gpuTimeoutArm = 0x800834A0u at line 154 (claim said 148 — the line MOVED, the value did not), .gpuTimeoutDeadlineVar = 0x800A5ADCu at 156, .gpuTimeoutFlagVar = 0x800A5AE0u at 157 — the same two globals the guest body writes. external/psxport/runtime/psx/sync_overrides.cpp:138 still defines vsync_trap as trap_abort(c,'VSYNC',...) and line 211 still registers it on h.vsyncTrap, so porting the guest body faithfully would still hit an abort on its call to 0x80085900. tools/codemap.py --addr 800834A0 reports it PlatformHle-owned, not a scanned native, so the codemap agrees. NOT re-verified: the live recdep run cited in the original evidence (2276 dispatches) was not repeated — that needs a game run, and nothing about this claim depends on the COUNT. The falsifier is unchanged and untriggered: the port still runs GPU/OT DMA synchronously.
