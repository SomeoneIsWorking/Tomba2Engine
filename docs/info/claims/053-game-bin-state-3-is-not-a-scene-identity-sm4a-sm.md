---
id: C053
kind: claim
status: holds
created: 2026-08-21
tags: render,game-bin
depends: game/render/scene_kind.cpp#classifyGameStageScene
---

## Claim

GAME.BIN state 3 is not a scene identity: (sm4a,sm4c)=(1,3) is the hut interior, while (2,3) is the black-backed Save/Continue prompt

## Evidence

Dynamic retained-replay captures: hut-entry-door-freeze f1200 reads (1,3); save-prompt-black-screen f12887 reads (2,3). GAME.BIN RE identifies sm4a=2 as FUN_80106478 and states 3..8 as Save/Continue/Load/Quit. test_scene_kind exercises both selectors; post-fix runtime capture removes stale field geometry while retaining prompt glyphs/cursor.

## What would falsify it

A GAME.BIN disassembly or live state capture shows a hut at sm4a!=1, or a Save/Continue/Load/Quit prompt outside sm4a=2 states 3..8
