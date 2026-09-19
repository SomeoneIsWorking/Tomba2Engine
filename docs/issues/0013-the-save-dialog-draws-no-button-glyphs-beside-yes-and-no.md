---
id: 13
title: The save dialog draws no Cross/Circle button glyphs beside Yes and No
status: open
symptom: the console reference draws the ✕ and ○ pad glyphs to the right of "Yes" and "No" in the Save? dialog; the product draws neither, leaving a magenta smear where the ✕ belongs
state_items: S004
tags: render,2d,ui,picture-oracle,save
created: 2026-09-19
updated: 2026-09-19
---

## Reported by the user, then reproduced

USER 2026-09-19: "I can see save menu for example not being like the oracle". This is that, found
and localised.

Nothing here could have caught it before. `tools/oracle_compare.py` reads guest RAM and reports 0
divergences on this exact route — correctly, because a missing glyph writes no different guest
state. `tools/vram_oracle.py` compared two blank buffers on the native path and called it a pass
(psxport issue 0121). The instrument that found it is `tools/picture_oracle.py`, on its first run
against a recorded route.

## How it was reached

`tools/oracle_compare.py --route replays/bugs/save-card-pages.pad --route-from 150` drives BOTH
cores with the user's own recorded save route instead of a scripted one, from the free-roam
checkpoint. The route runs **1,699 game frames across 18 checkpoints with 0 divergences**, so the
simulation reaching the dialog is identical on both cores; the picture at that state is not.

It then stops with an honest refusal rather than a divergence: `the guest passed its vblank gate
again after 4 VBlank(s) with task 0 still running`. The memory-card access legitimately runs its
logic across frames, which the strict per-frame alignment cannot align. Comparing past the card
write needs that boundary handled; it is not a product defect.

## What was measured

At the "Save? / Yes / No" dialog (recording frame 1699), both sides 320x224:

```
[picture] save-card-pages-1545f: 42348/71680 pixels differ (59.08%), 280/280 tiles touched
[picture]   worst tiles (192,192):255, (160,160):248, (176,160):237, (144,176):237
```

Whole frame: mean absolute difference 31.1/765, **median 8**, 5,884 pixels (8.2%) past 96. Split by
region, the defect is not spread at all:

| region | mean abs diff | fraction past 96 |
|---|---|---|
| glyph column (x 175..205, y 80..120) | **165.7** | **0.412** |
| dialog box (x 120..205, y 55..125) | 42.4 | 0.112 |
| world outside the box (x 0..110) | 24.1 | 0.056 |

The glyph column differs about seven times as much as the world does. Looking at the two pictures:
the reference draws a white-ringed blue ✕ beside "Yes" and a white-ringed red ○ beside "No"; the
product draws neither, and carries a magenta smear where the ✕ belongs.

## What is NOT established

Which producer owns those glyphs, and whether the magenta smear is a mis-sampled CLUT for the same
sprite or unrelated content showing through. The world outside the box also differs (24.1 mean,
5.6% past 96) and that is not attributed either; some of it is the expected renderer difference
between a native producer and the reference's rasterisation of the guest's command stream, and how
much is unknown.

`docs/unported-render-inventory.md` is the place to check for an unported 2D producer first.
