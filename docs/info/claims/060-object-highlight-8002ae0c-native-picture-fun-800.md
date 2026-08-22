---
id: C060
kind: claim
status: holds
created: 2026-08-22
tags: render,tomba2
depends: game/render/object_highlight.cpp#Render::objectHighlightRender, game/render/render_walk.cpp#Render::fieldObjectsRender
reconfirmed: 2026-08-22 19:15:24
verified_at: 2026-08-22 19:15:24
---

## Claim

object-highlight-8002ae0c-native-picture: FUN_8002AE0C has a reached state-native display producer whose same-binary native-gate activation changes pixels inside its reported screen boxes on bucket-softlock replay frame 255

## Evidence

generated/shard_2.c gen_func_8002AE0C; scratch/logs/gate-run-20260822-{190956,191128}.log; scratch/screenshots/highlight_exact_{on,off}_255.ppm; docs/producers/0x8002AE0C.md

## What would falsify it

Falsified if a same-source compiled A/B produces no pixels inside the producer-reported boxes, if the queue-A live route no longer reaches FUN_8002AE0C, or if generated/shard_2.c semantics differ from the native transform/cue recipe

## Re-confirmed 2026-08-22 19:15:24

Same-binary native highlight A/B on clean psxport 57a17a14: both legs md5 7823cb4d3cea advanced 321 frames; replay frame 255 changed 318 pixels, 44 inside reported boxes; 274 outside remain explicitly unresolved.
