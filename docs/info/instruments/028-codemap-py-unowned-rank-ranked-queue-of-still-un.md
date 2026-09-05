---
id: I028
kind: instrument
status: DISTRUSTED
created: 2026-07-30
distrusted_on: 2026-07-30
---

## Instrument

codemap.py --unowned-rank — ranked queue of still-unowned guest functions by recdep hotness

## Validated by

listed the known-unowned hot set (0x800834A0, 0x80040558, 0x80086288) AND correctly omitted five known-OWNED hot addrs (Mtx::identity 0x80051794, Trig::rcos 0x80083F50, Trig::rsin 0x80083E80, OverlayGt3Gt4::submitBlock 0x80146478, CollisionResolve::landOnObjectTop 0x8002423C) — i.e. proven able to give the OTHER answer, not just 'unowned' for everything

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-30

It LIED on its very first use and its own validation embedded the lie: it cites 0x800834A0 as a 'known-unowned hot addr', but that address is owned by PlatformHle as gpu_timeout_arm (external/psxport/runtime/psx/sync_overrides.cpp, wired from GameConfig::hle.gpuTimeoutArm in game/core/game_config.cpp). It filtered only on codemap's source-scanned index, which cannot see the PlatformHle table at all, so it put an already-owned address at the TOP of the port queue at 33,152 hits and dispatched a full porting session at a double-install. Instrument I024 had already recorded this exact blind spot for the recdep histogram; the new queue reproduced it. FIXED in tools/codemap.py (load_platform_hle_table + exclusion in --unowned-rank + ownership report in --addr) — see I029.

> Every result this instrument produced is suspect until it is re-validated.
