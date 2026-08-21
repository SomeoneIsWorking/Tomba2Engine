---
id: C057
kind: claim
status: holds
created: 2026-08-21
tags: render,tomba2,fallback-debt
depends: game/render/guest_gte_water_jet.cpp#replayGuestGt4Span, game/render/guest_gte_water_jet.cpp#waterJetWriterTap, game/game_tomba2.cpp
reconfirmed: 2026-08-21 14:14:51
verified_at: 2026-08-21 14:14:51
---

## Claim

FUN_8013D454 water-jet fallback replays exact packet-addressed guest-GTE output at logic time without display interpolation on the tracked replay

## Evidence

walk-dust-puff.pad true SBS oracle: B identifies PURE-ORACLE(interp+softGPU) and is byte-identical pre/final at f450/460/470/480/490/500/510/520; live samples each emit 2 GT4 packets with 8/8 exact depth hits and zero miss/stale; f450/f500 are zero-call and zero-pixel controls; c15_waterjet is exact vs landed impact in both panes. docs/findings/render.md.

## What would falsify it

Any tracked run where a non-water-jet FUN_80027768 caller replays packets, any accepted packet lacks its exact guest depth, any fallback item enters display interpolation, retained oracle B changes, an idle control changes, or the 0x8013D454 packet/output contract changes.

## Re-confirmed 2026-08-21 14:14:51

Reconfirmed against pinned psxport 3418a79b624765614f3f198dc1e89632e1e650f0 after clean Clang/Clang++ 22.1.8 build and CTest 4/4: walk-dust-puff.pad true SBS B identified PURE-ORACLE(interp+softGPU); f450/460/500/520 A+B captures are byte-identical to the retained 692b9b20 gate; live f460/f520 each replay 2 GT4 packets with depth=8/0/0 and interp=off; f450/f500 controls remain silent. Default ./run.sh selected 3418a79b with recomp up to date and reached the separately tracked card #121 key-order failure at f1950.
