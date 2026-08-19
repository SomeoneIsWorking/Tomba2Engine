---
id: 106
title: PSX/oracle path showed 16 rows below the cutscene letterbox — the display height was the framework's default, not the game's
status: done
labels: [render, psxport]
created: 2026-08-19
updated: 2026-08-19
---

USER 2026-08-19: 'PSX has a bogus segment below the cutscene black bars, this is an oracle bug'.

MEASURED: the strip is exactly rows 224..239. The guest's own letterbox rects sit at rows 0..11 and 212..223 — flush to the bottom of a 224-line screen — and game/render/cine_bars.cpp already carried the RE note that the guest bars are 'sized for a 320x224 PSX display'. The new 'disp' debug-server command showed why the port did not know that: Tomba!2 issues 16,061 GP1 writes in a session and NOT ONE is 05, 07 or 08, so s_disp_h was psxport's 240-line default rather than a decoded value. We presented 16 rows of framebuffer no console scans out.

FIX (psxport 71021367): GameConfig::guestDisplayHeight — the port declares what the game really scans out; Tomba2 sets 224 (game/core/game_config.cpp). Applied to the GUEST-SOURCED paths only (gte/psx), which are the ones claiming to show the console's picture; the native renderer owns its own frame and keeps 240 — USER: 'cine_bars is PC right? PC is fine, oracle isn't'.

VERIFIED: AUTO_SKIP free-roam, same instance — guest path vkshot 320x224, native path 320x240 unchanged, and the run logs the difference once ('guest render path presents 224 lines, not 240'). NOT re-verified on the cutscene picture itself: the fix removes rows 224..239, which is precisely where the strip was, but nobody has re-shot that scene.
