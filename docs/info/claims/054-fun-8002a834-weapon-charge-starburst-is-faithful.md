---
id: C054
kind: claim
status: holds
created: 2026-08-21
tags: render,tomba2
depends: game/render/fx_swing.cpp#Render::swingStarburstRender, game/render/render_walk.cpp#Render::fieldObjectsRender, game/render/mesh_quads.cpp#meshQuadRecordsEmit, psxport.pin
reconfirmed: 2026-08-21 11:48:51
verified_at: 2026-08-21 11:48:51
---

## Claim

FUN_8002A834 weapon-charge starburst is faithfully owned by a read-only display-pass producer built from controller state, with the pure software-oracle pane retained unchanged.

## Evidence

RE: generated/shard_0.c gen_func_8002A834 and generated/shard_5.c gen_func_80027768. Runtime: scratch/logs/c14_charge_fresh.log reports B=PURE-ORACLE(interp+softGPU), native 10 copies/60 quads beginning f667. Saved pre-A vs fresh A: 0-pixel deltas f650/f660, then 843/642/939/1001 pixels at f670/f680/f690/f700 within the starburst footprint; B SHA256-identical before/after at all six frames. Durable summary: docs/findings/render.md #14.

The bounded route reached 261/484 owned addresses; the claim is scoped to this exercised controller path.

Final combined-tree rerun: scratch/logs/c14_charge_combined.log contains the same 38 swingfx lines byte-for-byte, exits cleanly at f705, and its six B captures remain byte-identical. Combined A also contains the independent #55/#72 native graphic and is therefore not used as a whole-frame #14 isolation hash.

Repin verification against definitive psxport `692b9b20e3d4a6194452522060fd2657c2235f40`: after the preliminary 9f framework forced a full recompilation-substrate re-emission, the final pin received another clean Clang 22.1.8 rebuild. `replays/bugs/weapon-charge-starburst.pad` again exited cleanly at f705. All 38 `swingfx` lines and every A and B capture at f650/f660/f670/f680/f690/f700 are byte-identical to the retained 9f run. Evidence: `scratch/logs/repin_692_c14.log`, `scratch/screenshots/repin_692_c14_f*_{A,B}.ppm`.

## What would falsify it

Any corrected replays/bugs/weapon-charge-starburst.pad run where the software-oracle starburst appears but native A does not, where native emits before controller activation, where B changes between producer-disabled and producer-enabled builds, or any change to the producer dispatch, transform contract, packed-mesh decoder, or oracle mode.

## Re-confirmed 2026-08-21 11:48:51

Post-landing clean Clang 22.1.8 build and true-oracle replay on psxport 692b9b20 exited at f705; all 38 swing telemetry lines and six A/B frame pairs match retained evidence exactly.
