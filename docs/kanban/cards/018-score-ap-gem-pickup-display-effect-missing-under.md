---
id: 18
title: Score/AP-gem pickup display effect missing under pc_render
status: todo
labels: [render]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22: 'score display effects are missing also such as picking up an AP gem'. Picking up an AP gem should produce a score/points display effect; it does not render under pc_render. Same family as #12 (torch flame) and #14/#15 (weapon charge/impact) - an effect layer with no native producer since the break-first render rebuild, when pc_render stopped walking the guest OT. Repro: reach a gem and collect it, shot the frames right after contact, compare pc_render against psx_render IN ONE PROCESS via the renderpsx REPL toggle on the DEFAULT leg (GATE-vs-ORACLE is blind to tap-based producers). FIRST SUSPECT, and it is a strong one: the unowned 40-slot effect pool at 0x800EC188 -> FUN_8003D23C (a 7-word textured-tri emitter), already fully RE'd and written up in docs/findings/render.md as the prime remaining suspect for exactly this class of short-lived pickup/impact effects. Check that before RE'ing anything new. Fix pattern is the tap: run the gen body, re-derive the quads host-side from the contract it publishes (see game/render/fx_sprite.cpp).
