---
id: C062
kind: claim
status: holds
created: 2026-08-25
tags: render,fps60,cubetext
depends: game/render/cube_text_banner.h#CubeTextBanner::pictureBuildAllowed, game/render/cube_text_banner.cpp#CubeTextBanner::render
reconfirmed: 2026-08-25
verified_at: 2026-08-25 00:15:20
---

## Claim

CubeTextBanner's native picture builder is structurally disabled during temporal world-capture-only passes, while remaining enabled for ordinary presentation.

## Evidence

tests/test_cube_text_banner_policy.cpp passes all four oracle/capture truth-table cases; the combined Clang build links tomba2_port against the isolated frame-boundary framework fix. Runtime presentation-ledger confirmation remains issue #2's closing gate.

## What would falsify it

A call to CubeTextBanner::render emits any RenderQueue item while Fps60::mWorldCaptureOnly is true, or a second picture-building path bypasses CubeTextBanner::pictureBuildAllowed.

## Re-confirmed 2026-08-25

Bounded headless presentation-ledger run scratch/logs/gate-run-20260825-001241.log: after the cold warp, the former 164 CubeText banner world quads are absent. The remaining f3016 mismatch is exactly 2 world quads, both node 0x800EDE28; combined Clang unit/build evidence remains green.
