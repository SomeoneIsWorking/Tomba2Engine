---
id: C067
kind: claim
status: holds
created: 2026-08-27
tags: repl,warp,presentation,frame-boundary
depends: game/core/frame_driver.cpp#TombaFrameDriver::stepFrame, tools/verify_native_frame_contract.py#FRAME_ORDER
---

## Claim

A standalone cold dev warp is a frame-boundary transaction: Tomba 2 presents the pending old-scene capture, begins the new capture epoch, applies the game-owned warp, then runs destination guest work; SBS never enters this standalone phase.

## Evidence

The current title owner is game/core/frame_driver.cpp#TombaFrameDriver::stepFrame. Its source order is presentation commit, capture reset, applyArmedStandaloneWarp, then game.pcSched.step. tools/verify_native_frame_contract.py checks those title-local tokens and rejects reversed order. The bounded combined-product log scratch/logs/gate-run-consumed-flush-20260825.log records f3015 presenting the complete pending old scene before the warp and f3016 starting destination capture without stale items.

## What would falsify it

TombaFrameDriver applies an armed warp before committing the pending frame, destination guest work runs before the warp, the unarmed path changes phase ordering, or SBS services the standalone REPL warp phase.
