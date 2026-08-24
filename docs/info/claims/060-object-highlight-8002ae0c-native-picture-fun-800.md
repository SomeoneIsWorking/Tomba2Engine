---
id: C060
kind: claim
status: holds
created: 2026-08-22
tags: render,tomba2
depends: game/render/object_highlight.cpp#Render::objectHighlightRender, game/render/render_walk.cpp#Render::fieldObjectsRender
reconfirmed: 2026-08-22 19:21:12
verified_at: 2026-08-22 19:21:12
---

## Claim

object-highlight-8002ae0c-native-picture: FUN_8002AE0C has a reached state-native display producer whose same-binary native-gate activation changes pixels inside its reported screen boxes on bucket-softlock replay frame 255

## Evidence

generated/shard_2.c gen_func_8002AE0C; scratch/logs/gate-run-20260822-{190956,191128}.log; scratch/screenshots/highlight_exact_{on,off}_255.ppm; docs/producers/0x8002AE0C.md

## What would falsify it

Falsified if a same-source compiled A/B produces no pixels inside the producer-reported boxes, if the queue-A live route no longer reaches FUN_8002AE0C, or if generated/shard_2.c semantics differ from the native transform/cue recipe

## Re-confirmed 2026-08-22 19:15:24

Same-binary native highlight A/B on clean psxport 57a17a14: both legs md5 7823cb4d3cea advanced 321 frames; replay frame 255 changed 318 pixels, 44 inside reported boxes; 274 outside remain explicitly unresolved.

## Re-confirmed 2026-08-22 19:21:12

Post-commit 0948fce authoritative Clang CTest passes 6/6. Same-binary replay gate advances both legs 321 frames; 318 pixels change and 44 lie inside conservative producer boxes. The remaining 274 are explicitly unresolved and excluded from the localization claim.

## Re-confirmed 2026-08-22 19:53:00

Final-composited presentation captures were also tested with the same binary. At logical 320x240 they change 326 pixel blocks: 63 within the three producer bounds and 263 outside. Changing capture phase did not isolate the spill, so the 263 remain explicitly unresolved and excluded from the claim.
