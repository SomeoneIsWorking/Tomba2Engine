---
id: C059
kind: claim
status: holds
created: 2026-08-22
tags:
depends: game/core/tomba_runtime.cpp, game/core/main.cpp#main, game/core/game_hooks.cpp
---

## Claim

Tomba! 2 installs a process-lifetime TombaRuntime and directly owns context lifecycle, boot initialization, and override registration through GameRuntime inheritance; the corresponding legacy hook slots are null.

## Evidence

Clang build of tomba2_port passed; CTest 5/5 including clang-format, clang-tidy, and source caps; tools/gate.py boot advanced 401 frames past prologue to stage=8010637C sm48=2 with exit 0 and 0 unknown CVars. Source inspection shows main installs TombaRuntime before Game construction and compatibilityHooks omits ctxCreate, ctxDestroy, bootInit, and registerOverrides.

## What would falsify it

A framework call reaches any of those four operations through GameHooks, TombaRuntime is installed after a Game is constructed, or the 400-frame boot gate no longer reaches GAME stage 8010637C state 2.
