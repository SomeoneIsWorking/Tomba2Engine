---
id: C064
kind: claim
status: holds
created: 2026-08-25
tags: render,fps60,area-load
depends: external/psxport/runtime/recomp/render_queue.cpp#RenderQueue::flush, game/render/cube_text_banner.cpp#CubeTextBanner::render
---

## Claim

The combined pending-frame warp boundary, CubeText capture guard, and RenderQueue consumed-flush guard eliminate the Area 21 cold-warp presentation drop: f3016 captures zero items and the ledger reports zero dropped layers through f3614.

## Evidence

Bounded real headless run scratch/logs/gate-run-consumed-flush-20260825.log, build edaa13a-dirty+psxport-1b87bf0a-dirty: prior f3016 8/6 becomes captured n=0; first destination capture is 131 world items at f3336; run-end 3614 reconciled, 0 dropped. PID 187070 exited normally after one scripted newgame/warp run.

## What would falsify it

A repeat of the same newgame; skip 3000; warp 21 0; skip 600 sequence captures any item at f3016, reports a dropped layer before f3614, or fails to resume destination world capture at f3336.
