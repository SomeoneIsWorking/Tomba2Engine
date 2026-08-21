---
id: C056
kind: claim
status: holds
created: 2026-08-21
tags: render,tomba2,effects
depends: game/render/fx_impact.cpp#Render::impactPlumeRender, game/render/render_walk.cpp#Render::fieldObjectsRender, game/render/mesh_quads.cpp#Render::meshQuadRecordsEmit
reconfirmed: 2026-08-21 12:07:24
verified_at: 2026-08-21 12:07:24
---

## Claim

FUN_800288AC weapon-impact plume is faithfully display-pass owned for the tracked composite and direct-node replays

## Evidence

replays/bugs/weapon-impact-bucket.pad SBS oracle run: B identifies PURE-ORACLE(interp+softGPU) and stays byte-identical pre/post at f646..f660; native A gains 588/826/492/206 pixels at f652/f654/f656/f658 inside impactfx bbox and visibly restores the same blue-white plume. bucket-softlock.pad reaches direct 0x800288AC nodes f298..f332. docs/findings/render.md.

## What would falsify it

Any corrected tracked replay where pure software-oracle B draws the plume but native A does not, native geometry leaves impactfx reported bounds, B changes because of the producer, or the controller/writer transform, cue, palette, ordering, or dispatch contract changes.

## Re-confirmed 2026-08-21 12:07:24

Reconfirmed after final combined-source build and full 4/4 CTest: c15_post SBS B is PURE-ORACLE(interp+softGPU), all eight B captures are exact vs c15_pre, native A gains the plume within impactfx bounds, and bucket-softlock directly reaches 0x800288AC.
