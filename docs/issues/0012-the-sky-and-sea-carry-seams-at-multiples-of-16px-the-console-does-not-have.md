---
id: 12
title: The product's sky and sea carry seams at exact multiples of 16 px that the console does not have
status: open
symptom: over a correctly aligned 320x224 comparison the product's sky shows brightness discontinuities at columns 16, 80, 96, 112, 128, 144, 160 where the console's sit only on content edges; a user reported the save menu "not being like the oracle"
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

### 2. RETRACTED — "the world is drawn 7 pixels high" was my own misalignment

The first version of this issue reported a second defect: the land/horizon boundary at row 87 in the
product against row 94 on the reference, over a 92.39% whole-frame difference. That was wrong, and
the correction is worth keeping because the mistake is easy to repeat.

The native render path deliberately presents MORE rows than the console scanned out. This title
declares `guestDisplayHeight = 224` in `game/core/game_config.cpp` and the framework keeps drawing
the framework's 240-line default, on a decision already recorded in `psxport
runtime/psx/gpu_native.cpp` (USER 2026-08-19: "PC is fine, oracle isn't"). Comparing a 240-row
product frame against a 240-row reference frame carrying 8 black rows top and bottom therefore
measures a difference nobody considers a defect, and it accounts for the whole apparent offset: the
fit was `native_row = console_content_row * 1.008 - 8.5`, which is unit scale and the reference's
own top border, not a projection error.

With the product cropped to the rows it itself reports as scanned (`guest_scan=` on the shot reply,
from the GPU state) and the reference asked for its own active area (`crop_overscan=smart`, which
publishes 320x224 and so agrees with the declared 224 independently), the comparison is 320x224 on
both sides and:

```
[picture] played-400f: 33786/71680 pixels differ (47.13%), 280/280 tiles touched — spread
```

mean absolute difference 24.7/765, **median 0**, 44,239 of 71,680 pixels within 8, and no whole-pixel
shift improves it. Most of the picture matches exactly. 4,528 pixels (6.3%) differ by more than 96,
concentrated in the ground, and those are not yet attributed.

The alignment is taken from what each core reports about itself, never from fitting the two pictures
to each other — a fitted offset would have been the tool finding the answer that made it pass.

## Why this is not a widescreen or interpolation defect

It was measured with enhancements OFF, at 320x224 against a 320x224 reference, so it is in the 4:3
base picture. Widescreen coverage (S005) and interpolation (S006) sit on top of it.

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
