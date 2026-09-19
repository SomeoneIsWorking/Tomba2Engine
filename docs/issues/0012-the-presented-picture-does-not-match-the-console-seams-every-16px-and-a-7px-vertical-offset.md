---
id: 12
title: The presented picture does not match the console — sky seams every 16 px and the world drawn 7 px high
status: open
symptom: 70,958 of 76,800 pixels differ (92.39%) between the product's presented frame and the Beetle console reference's at the same state, with identical guest RAM; a user reported the save menu "not being like the oracle"
state_items: S004, S005
tags: render,picture,oracle,widescreen
created: 2026-09-19
updated: 2026-09-19
---

## How this went unnoticed

`tools/oracle_compare.py` reads guest RAM. It reports 405 checkpoints and 0 divergences on this
exact route, in all three legs — enhancements off, widescreen on, fps60 on — and it is right. A
picture drawn with the wrong seams, at the wrong vertical offset, or with the wrong colours writes
exactly the same guest state as a correct one, so no RAM number can move.

`tools/vram_oracle.py` compares the guest GPU command feed rasterised into emulated VRAM. On the
NATIVE render path, which is the shipping one, the picture is not produced that way, so both dumps
came back blank and it printed `differing 0/524288 (0.00%)` as a pass. It now refuses that case by
name. See psxport issue 0121.

`tools/picture_oracle.py` is the instrument that can see this, and this is its first run.

## What was measured

2026-09-19, product built against psxport `59fc724c`, 400 frames into the title's own gameplay route
from the free-roam checkpoint, both cores presenting 320x240 (the reference is asked for
`crop_overscan=static`, its own horizontal-only crop, so it publishes its active display area rather
than the padded 350-wide scanline):

```
[picture] played-400f: 70958/76800 pixels differ (92.39%), 300/300 tiles touched — spread
```

Same scene, same moment: mean RGB is (73.67, 138.29, 116.71) against (68.14, 129.03, 112.52), and
the guest RAM at this state is byte-identical. Two separate defects account for it.

### 1. The sky and sea carry seams at exact multiples of 16 px

Over the flat sky band (rows 8..40, no geometry), the column-to-column change in row-mean brightness
spikes at columns **16, 80, 96, 112, 128, 144, 160** — 54 spikes above 4/255. The reference's spikes
over the same band sit at 19-25, 155-158 and 240-249, which are real content edges (clouds, the
palm), 36 of them. A regular 16-pixel period is not content; it is the tiling the backdrop is drawn
with, showing its seams.

### 2. The world is drawn 7 pixels high

The land/horizon boundary — the first strongly-green row — is at **row 87 in the product and row 94
on the reference**. The best whole-frame integer translation is (0, +3), which reduces the mean
absolute difference from 105.3 to 78.4 and does not remove it, so this is not a pure translation of
the whole picture: the world geometry is offset while the 2D content is not.

## Why neither is a widescreen or interpolation defect

Both were measured with enhancements OFF, at 320x240 against a 320x240 reference. They are in the
4:3 base picture. Widescreen coverage (S005) and interpolation (S006) sit on top of this and cannot
be judged sound while the picture underneath them differs from the console by 92%.

## What is NOT established

The two cores are different renderers and a small evenly-spread difference is expected: the
reference rasterises the guest's command stream at native resolution, the product draws through
native producers. The tool reports this difference as `spread` rather than `concentrated` because
the seams touch every tile — that classification is about shape, not about whether it is a defect.
The two findings above are the ones with a measured mechanism; the remaining difference is not yet
attributed, and no other scene kind has been compared this way.

The user's original report was the SAVE MENU specifically. That scene is at ~frame 1690 of
`replays/bugs/save-card-pages.pad` and is not reachable from the oracle's scripted route, so it has
not been picture-compared yet. Reaching it needs the route extended; the defects above were found on
the way there and are in the base picture the save menu is drawn over.
