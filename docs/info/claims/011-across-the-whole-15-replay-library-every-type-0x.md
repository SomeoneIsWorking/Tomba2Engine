---
id: C011
kind: claim
status: falsified
created: 2026-07-28
tags: render
falsified_on: 2026-08-06
---

## Claim

Across the whole 15-replay library, every type-0x20 render fn the nofx census reaches now has a producer.

## Evidence

Full sweep 2026-07-28, each replay sized to its own pad length, headless: all 15 exit 0, zero abort/fatal/recomp-MISS. Skipped-fn union is 0x8002AB5C (terrain), 0x8013CDD4 (widescreen margin), 0x800288AC + 0x8002BC9C (scoped FxMesh controllers), 0x8002A834 (SwingFx) — all owned by another route. 0x8013D454 has left the list.

## What would falsify it

any new replay or area that makes nofx name a fn with no owner; render_fns.py still lists static candidates no replay visits, so this is a statement about coverage, not completeness

## FALSIFIED 2026-08-06

FALSIFIED 2026-08-06 by measurement on the CURRENT build. psxport-era commit abf3cf9 ('Delete the GTE-register render taps; four layers are now honestly absent') deleted game/render/fx_mesh.cpp/.h, mesh_emit_tap.cpp and swing_fx.cpp/.h — the ONLY producer route for the shared effect-mesh writer FUN_80027768. The claim's own evidence line rests on three of its five skipped fns being 'owned by another route'; that route no longer exists. tools/codemap.py --addr now answers NO NATIVE OWNER for 0x800288AC, 0x8002BC9C, 0x8002A834, 0x80027768, 0x8013D828, 0x8013ED08, 0x8013EF58. LIVE CONFIRMATION, not a grep: PSXPORT_GATE=1 pc_render, replays/bugs/bucket-softlock.pad, 460 frames headless, PSXPORT_DEBUG=nofx names 0x8002BC9C (node 800EE7B8) and 0x800288AC (node 800EEBF8) as skipped in area 0 on this build, alongside 27 live type-0x20 nodes of 152 walked per frame (ringcensus). See docs/unported-render-inventory.md item R1.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
