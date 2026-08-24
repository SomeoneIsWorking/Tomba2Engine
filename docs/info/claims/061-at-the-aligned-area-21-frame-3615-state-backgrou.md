---
id: C061
kind: claim
status: holds
created: 2026-08-24
tags: render,area21
depends: game/render/area21_sky_gradient.cpp#Render::area21SkyGradientRender, game/render/area21_sky_gradient_policy.h#Area21SkyGradientPolicy::bands
---

## Claim

At the aligned Area 21 frame 3615 state (background 21, variant 1, phase 1, pitch -175), the native 0x8010BB64 producer restores the coherent four-band sky that is absent with its gate off; it is close to the PSX-render reference but is not pixel parity.

## Evidence

Fresh 2026-08-24 one-binary headless gate: scratch/logs/area21_fresh_native_on_20260824.log, area21_fresh_native_off_20260824.log and area21_fresh_psx_reference_20260824.log record identical state/frame; scratch/screenshots/area21_fresh_native_off_on_diff.png measures 53907/76800 changed (53842 >8); native_vs_psx_reference measures 26853 exact / 20094 >8; root visual verdict: ON coherent and close, OFF loses background.

## What would falsify it

A repeat at the same frame and four guest inputs shows OFF retains the sky, ON no longer follows FUN_8010BB64 geometry/colors, or the aligned reference is no longer the PSX-render path.
