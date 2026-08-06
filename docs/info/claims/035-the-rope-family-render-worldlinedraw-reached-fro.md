---
id: C035
kind: claim
status: holds
created: 2026-08-06
tags: 
depends: game/render/fx_line.cpp#worldLineDraw, game/render/render_walk.cpp
---

## Claim

The ROPE FAMILY (Render::worldLineDraw, reached from ropeAnchorRender / ropeChainRender / tetherLineRender) actually PUTS PIXELS ON THE PRESENTED SCREEN under pc_render — this is a pixel measurement with a negative control, not a primitive count

## Evidence

A/B/C/D legs, four SEPARATELY-BUILT binaries from one isolated tree (never the shared checkout), distinct md5s, each build exit-0 checked. Same replay (bucket-softlock.pad), same frames, headless, PSXPORT_GATE=1 pc_render, PSXPORT_PRESENT_SHOT_AT=440..485 -> gpu_vk_present_shot 960x720. IN-BAND LEG PROOF (not the pixels being measured): leg B logs 666 worldLineDraw calls + 152 shockwave calls, leg A logs the 598 of those 666 that survive the behind-camera reject + the same 152 — equal invocation counts prove both legs reached the same scene. RESULT A-vs-B: 1584/1197/927 changed px of 691200 at presents 440/445/450, bbox 41x263 / 41x206 / 29x164, tracking right as the camera pans. NEGATIVE CONTROL: leg B (producers deleted) does not show them, and the same instrument reports 0 on presents 455-485 where the producer is not called at all (ropeline frame stamp: last call f453; independently bracketed — call count saturates at 666 by f456). ISOLATION: D-vs-A (rope only) reproduces 1584/1197/927 exactly, so the whole diff is the rope. Eyeball evidence scratch/lineclass/evidence/present_44*_OFF_ON_DIFF.png: the diff mask is one continuous thin curved line.

## What would falsify it

a pc_render present in a rope scene where the rope is absent while ropeline reports worldLineDraw calls at that same frame stamp; or a USER eyeball showing the rope in the wrong place (position was NOT checked against psx_render pixels here — only the producer's own endpoints were, and that was a previous session's measurement, not this one)
