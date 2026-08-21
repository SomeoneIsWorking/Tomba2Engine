---
id: C057
kind: claim
status: holds
created: 2026-08-21
tags: render,tomba2,fallback-debt
depends: game/render/guest_gte_water_jet.cpp#replayGuestGt4Span, game/render/guest_gte_water_jet.cpp#waterJetWriterTap, game/game_tomba2.cpp
---

## Claim

FUN_8013D454 water-jet fallback replays exact packet-addressed guest-GTE output at logic time without display interpolation on the tracked replay

## Evidence

walk-dust-puff.pad true SBS oracle: B identifies PURE-ORACLE(interp+softGPU) and is byte-identical pre/final at f450/460/470/480/490/500/510/520; live samples each emit 2 GT4 packets with 8/8 exact depth hits and zero miss/stale; f450/f500 are zero-call and zero-pixel controls; c15_waterjet is exact vs landed impact in both panes. docs/findings/render.md.

## What would falsify it

Any tracked run where a non-water-jet FUN_80027768 caller replays packets, any accepted packet lacks its exact guest depth, any fallback item enters display interpolation, retained oracle B changes, an idle control changes, or the 0x8013D454 packet/output contract changes.
