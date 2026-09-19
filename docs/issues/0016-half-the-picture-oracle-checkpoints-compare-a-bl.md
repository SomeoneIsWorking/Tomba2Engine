---
id: 16
title: Half the picture-oracle checkpoints compare a blank frame, so they cover nothing
status: open
symptom: of the six checkpoints in the most recent picture-oracle capture set, field and free_roam are entirely black on BOTH sides and game_stage has a 93%-non-black native frame against a completely black console one; only played-400f and the two save-card-pages checkpoints carry a picture on both sides
tags: render,picture-oracle,coverage,instrument
created: 2026-09-19
updated: 2026-09-19
---

## Symptom

Measured over `scratch/picture/*.png`, comparing each `<tag>.native.scanned.png` (320x224, the
comparable crop) against its `<tag>.console.png`. Non-black is any pixel whose channel sum exceeds
24, sampled on a 2x2 grid:

| checkpoint | native.scanned | console | verdict |
|---|---|---|---|
| `field` | 0% | 0% | **both black — compares nothing** |
| `free_roam` | 0% | 0% | **both black — compares nothing** |
| `game_stage` | 93% | 0% | **one side black** |
| `played-400f` | 99% | 99% | has content |
| `save-card-pages-1250f` | 93% | 93% | has content |
| `save-card-pages-1545f` | 93% | 93% | has content |

These are not stale. The three blank ones are the NEWEST files in the directory (14:21, against
13:45-14:12 for the three good ones), so they are the current state of the capture set, not leftovers
from an older run.

## This is a coverage hole, NOT a false pass

The picture oracle already guards against exactly this. `external/psxport/tools/oracle/picture.py`
computes `non_black` per image, defines `blank` as `non_black == 0`, and refuses at
`if picture.blank or picture.uniform:` with a message naming the non-black count and the total. Its
own header says the guard exists because on a native render path the picture never enters emulated
VRAM and an early version "compared two blank" images.

So these three checkpoints REFUSE. Nothing is being reported green that is actually black. The
defect is that three of six checkpoints produce no comparison at all, and a run that refuses half
its checkpoints is much weaker evidence than its summary line suggests.

## Why `game_stage` is the interesting one

`field` and `free_roam` are black on both sides, which is consistent with neither core reaching a
presented state at that checkpoint. `game_stage` is different: the product renders a full picture
(93% non-black) and the console leg renders **nothing**. That asymmetry says the console leg failed
to reach or failed to capture that state, which is a different fault from the route never getting
there. It should be diagnosed separately from the other two.

## What this does not say

Nothing here says the product's picture is wrong at these three states. It says they are UNCHECKED.
Do not convert this into a rendering issue without first getting a real console frame to compare.

## Where to look next

1. For `game_stage`, find out why the console leg captured black while the native leg captured a
   picture. The console capture path is the suspect, not the renderer.
2. For `field` and `free_roam`, determine whether the route reaches a presented state at all at
   those checkpoints. If it does not, the checkpoint definition is wrong and should either be moved
   or removed rather than left refusing on every run.
3. Consider making the run's summary state coverage explicitly — "3 of 6 checkpoints compared, 3
   refused blank" — so a half-covered run cannot read as a clean one at a glance. The guard already
   refuses correctly; what is missing is the denominator in the verdict.

## Related

Issue 0013 (fixed) and 0014 (open) both came from picture-oracle findings on the checkpoints that
DO carry a picture, which is the evidence that this instrument earns its keep where it can see.
