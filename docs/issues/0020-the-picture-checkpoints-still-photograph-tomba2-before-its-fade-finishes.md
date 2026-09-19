---
id: 20
title: The picture checkpoints still photograph Tomba! 2 before its fade finishes, and the settled difference is ~48% symmetric
status: open
symptom: at `field` and `free_roam` the product is brighter than the console on 72.3% and 52.6% of pixels and darker on only 4.2% and 11.7%, with a modal signed difference of exactly +16 (two 15-bit colour steps). Sixty or more frames later the same route is symmetric — 49.2%, 48.0%, 48.7%, 47.9% positive — so the asymmetry is the product finishing its fade ahead of the console, not a renderer brightness defect
state_items: S005
tags: oracle,picture,checkpoint,fade
created: 2026-09-20
---

## This title produced two picture comparisons today where it had produced none

psxport `bb217d0f` changed `advance_to_presented` to require a SCENE rather than merely a non-blank
frame. Before it, all three of Tomba! 2's checkpoints refused as washed out — `game_stage`'s
reference 99.53% one colour, `field`'s product 51.15%, `free_roam`'s product 76.41% — because the
first non-blank frame of a fade-in is the fade. After it, `field` and `free_roam` compare.
`game_stage` still refuses, and correctly: its reference was advanced 130 frames further than the
product to reach a scene, and those are real game frames that move declared state.

## The brightness reading, and why it is NOT a renderer defect

The first measurement looked like a clean finding:

| checkpoint | product brighter | darker | modal signed difference |
|---|---|---|---|
| `field` | 72.3% | 4.2% | **+16**, 68% of all differing channels |
| `free_roam` | 52.6% | 11.7% | **+16**, 35% |

94.7% and 81.1% of differing channel samples were POSITIVE, and the product/console channel ratio's
deciles were pinned at 1.125 across the middle of the distribution. That is the shape of a
systematic brightening, and the obvious reading was a vertex-colour or 15-bit expansion defect.

**Falsified by driving further.** `--play 240 --frame-step 60` compares four more state-aligned
frames on the same route, none refused:

| frame | positive share of differing channels | product brighter / darker |
|---|---|---|
| `field` | 94.7% | 72.3% / 4.2% |
| `free_roam` | 81.1% | 52.6% / 11.7% |
| played f60 | 49.2% | 38.2% / 39.2% |
| played f120 | 48.0% | 16.5% / 19.1% |
| played f180 | 48.7% | 40.5% / 43.3% |
| played f240 | 47.9% | 29.1% / 32.1% |

The asymmetry does not shrink gradually — it is simply gone, and the modal difference stops being
+16. A renderer that brightened everything would brighten these frames too. So the +16 is the
product's fade completing about two 15-bit steps ahead of the console's at the moment the checkpoint
fires, and `docs/issues/0018`'s "mean luminance and per-channel means match, so it is not
brightness" is not in conflict: it was measured at the OLD stopping point, a different frame.

## What this issue actually is

**"Not washed out" is necessary but not sufficient.** A frame can be past the fade's dark phase and
still be finishing it. The pixel heuristic cannot tell, and it should not have to: this title already
declares the fade sequencer's outer state and its ramp level among its `picture_decisive` ranges.
Those agreed at every checkpoint, so the GUEST fade is aligned while the presented result is not,
which is the same shape as Spyro's `settled_play` problem (that repo's issue 0126) and wants the
same answer — a checkpoint driven to a settled state by a guest quantity, owned by the title rather
than inferred from pixels by the framework.

## The number that is left, and is not explained

At the settled played frames the difference is symmetric and still large: 21.01% of pixels a
different COLOUR at f120 over 171/280 tiles, 40.60% at f240 over 274/280. Symmetric and spread is
the signature of a sampling/placement difference rather than a missing or mislaid object — issue
0019's ~3px framing offset is a candidate and is not established as the whole of it. That is the
open question; the brightness was a distraction this issue exists to stop the next session
rediscovering.

## Next

1. Give the checkpoints a settled-state predicate from the fade ramp the title already declares,
   so the shutter fires after the fade rather than after the darkness. Framework side is done.
2. Re-measure the settled difference from those checkpoints and take it against issue 0019.
