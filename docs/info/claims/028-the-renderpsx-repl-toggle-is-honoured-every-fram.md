---
id: C028
kind: claim
status: falsified
created: 2026-08-05
tags: render
depends: game/render/render_walk.cpp#areaCacheTrustTick
falsified_on: 2026-08-20
---

## Claim

The 'renderpsx' REPL toggle IS honoured every frame, in both directions; the area-cache trust latches (Render::mSceneTableTrusted/mBackdropTrusted) are ticked once per logic frame from Engine::drawOTag BEFORE the render-mode branch, so a mid-scene switch out of psx_render repaints the full pc picture. This REPLACES the old 'honoured per SCENE ENTRY' belief, which was two effects misread as one latch: a REPL shot reads the PREVIOUSLY presented frame (use run 2, not run 1), and the trust latches used to tick only on the pc_render branch, which permanently suppressed the backdrop + scene table after a psx->pc flip.

## Evidence

kanban #41 close, 2026-08-05: PSXPORT_DEBUG=areatrust read sceneTable=0 backdrop=0 with ent+6=132 bg+10=36 on the toggled leg vs 1/1 with the SAME guest bytes on a pure-pc leg (pre-fix); preseqobj counted 0 of 704 RQ_BACKGROUND items. Post-fix the toggled frame is 0/76800 px against a pure-pc boot leg at the same logic frame (pre-fix 71401/76800), and pc->psx is 0/76800 against a boot-time PSXPORT_RENDER_PSX=1 leg from the 2nd frame on. Both boot paths bit-identical to the pre-fix binary.

## What would falsify it

a mid-scene renderpsx flip in any OTHER area, in a window, or across a scene transition that does NOT reproduce the boot-time reference — only area 0 free-roam, headless, was measured

## FALSIFIED 2026-08-20

USER 2026-08-20 reports that selecting GTE/PSX during live gameplay inside the first hut freezes everything. C028 proved only area-0 headless picture equivalence and explicitly named a window/other-area live-switch failure as its falsifier; it did not prove gameplay progress or windowed switching.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
