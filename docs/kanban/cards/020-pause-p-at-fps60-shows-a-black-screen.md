---
id: 20
title: Pause (P) at fps60 shows a black screen
status: todo
labels: [render,fps60]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22: pressing P to pause while fps60 is ON shows a BLACK SCREEN. At 30fps pause presumably holds the last frame correctly (user reports this as an fps60-specific fault). MECHANISM TO CHECK FIRST: fps60 presents by re-running the world pass one frame behind under lerped input chokes into an isolated sink, then merging with verbatim 2D (see the fps60 entry in docs/findings/render.md). A PAUSE stops the game advancing, so the interpolator's two endpoints stop being refreshed - if the paused present keeps consuming the interp path it will lerp between a stale pair, or between a valid frame and an EMPTY sink, and an empty sink presents as black. So the suspect is the pause path presenting through the interpolated pass instead of re-presenting the last REAL frame. Related known code: the debug/pause loop calls PadInput::pumpHostInput rather than serviceFrame precisely so the pad clock does not advance while frozen (external/psxport/runtime/recomp/native_boot.cpp) - check whether the fps60 present is correspondingly frozen, or whether it keeps running with nothing feeding it. Repro: run windowed or drive the pause via the debug server, toggle fps60 on, pause, shot. Fix must NOT be an anchor/stamp special case (banned by the CLAUDE.md lerp clause) - the correct behaviour is that a paused frame is simply the last real frame presented through the same path.
