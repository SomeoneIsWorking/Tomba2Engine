---
id: 21
title: The opening cutscene presents every in-between frame a whole frame early, so it runs at 30 fps inside the 60 fps present
status: open
symptom: across the opening narration cutscene the in-between present is byte-identical to the NEXT real frame and does not move when the interpolation factor is forced to 0, on content whose two real endpoints are 0.67 apart
state_items: S005
tags: tomba2,fps60,cutscene,interpolation
created: 2026-09-22
updated: 2026-09-22
---

## Finding

Run the same replay twice, once at the product's own interpolation factor and once with
`PSXPORT_FPS60_TFORCE=0`, and every pixel whose two real endpoints differ has to say which endpoint
it landed on. Content that is interpolated sits at the PREVIOUS endpoint when the factor is forced
to 0. Content that is not interpolated is drawn from the current update's queue in both presents, so
it sits at the NEXT endpoint whatever the factor is — a whole frame early, every frame.

Measured 2026-09-22, `replays/bugs/walk-dust-puff.pad`, `aspect=1 fps60=1`, 428x240, with all 307
shared real frames byte-identical between the two runs (so the factor was the only thing that
changed):

| capture | triples | responded to t | did NOT respond | neither |
|---|---|---|---|---|
| fences 1..307 (boot, opening cutscene) | 247 | 64.66% | **30.21%** | 5.13% |
| fences 600..900 (`PSXPORT_FPS60_DUMP_FROM=600`, gameplay) | 299 | 96.35% | 2.19% | 1.46% |

Those are two behaviours, not one number. Per triple:

- **Gameplay:** 143 of 299 triples are at or under 1% unresponsive, median 1.19%, and NO triple is
  wholly unresponsive. The residue concentrates in the lower right (row bands 8..11 hold 72.6%,
  column bands 9..11 hold 68.9%) — specific content, tracked separately.
- **The opening:** 71 of 247 triples are 99%+ unresponsive, in runs `51..85, 92..125, 127, 129`.
  Only 5 of those 71 are across a discontinuity where refusing to interpolate is correct. The other
  **66 are continuous and still did not respond.**

The three-way classification is what makes this readable. An earlier two-way version asking only
whether the two runs DIFFER at a pixel read 33% "invariant" spread evenly over the screen, which is
quantisation noise: a pixel can land on the same colour at both factors.

## The frames, not the statistic

At `mean per-pixel channel difference` between the two real endpoints (`A..C`):

| fence | A..C | product interp vs A | vs C | forced t=0 vs A | vs C | product == forced |
|---|---|---|---|---|---|---|
| 60 | 0.67 | 0.67 | **0.00** | 0.67 | **0.00** | yes, byte for byte |
| 70 | 2.36 | 2.36 | **0.00** | 2.36 | **0.00** | yes, byte for byte |
| 100 | 5.74 | 5.73 | 0.02 | 5.71 | 0.04 | no |

At fences 60 and 70 the in-between present is the next real frame, bit for bit, at both factors.
That is not a sub-pixel quantisation onto an endpoint and not a cut: the endpoints are 0.67 and 2.36
apart, which is continuous motion.

## What this is NOT

- Not a cut. The cut population is separated by the endpoint distance and counted (5 of 71).
- Not the whole capture. Gameplay over fences 600..900 interpolates; this is the opening.
- Not measured from a symptom. Nothing here reads a screenshot for judder; the discriminator is the
  forced factor, and the control is that all 307 shared real frames are byte-identical.

## Reproduce

```sh
PSXPORT_FPS60_DUMP_FROM=1 python3 tools/gate.py --debug fps60dump \
    replay replays/bugs/walk-dust-puff.pad          # then move scratch/framedump aside
PSXPORT_FPS60_TFORCE=0 PSXPORT_FPS60_DUMP_FROM=1 python3 tools/gate.py --debug fps60dump \
    replay replays/bugs/walk-dust-puff.pad
python3 external/psxport/tools/port/fps60_check.py --dir <product> --forced <forced>
```

## Next

Find which presentation path the opening runs through and whether it reaches
`tomba_fps60_world_pass` at all. `docs/codemap.md` names `game/render/effect_lerp.*` and
`game/render/fps60_worldpass.cpp` as the interpolation owners; the question to answer first is
whether the cutscene's producers capture prior/current state with provenance, or submit only the
current update — the latter would produce exactly this reading with no interpolation code at fault.
