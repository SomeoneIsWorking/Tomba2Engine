---
id: 14
title: 33,805 chrome groups are dropped on one route for want of a page scope
status: open
symptom: UiGroupCapture::route drops every 2D chrome group that no page scope claims, silently until 2026-09-19; 36,635 on one replay, of which 26,290 were the card menu's own chrome because that producer was never installed (issue 0015)
state_items: S004
tags: render,2d,ui,picture-oracle,scope
created: 2026-09-19
updated: 2026-09-19
---

## How this was found

Issue 0013 added the negative report to `UiGroupCapture::route`: until then a group that no page
scope claimed was dropped in silence. Its first run on one replay printed 36,635 drops. 2,830 of
those were 0013's save prompt and are now drawn. **33,805 remain.**

This is one route. The number for the game is not known and is certainly larger.

## What is being dropped

`grep DROPPED` on `scratch/logs/gate-replay-20260919-141053.log`, 2,400 frames:

| count | template | bucket | placement | what it looks like |
|---|---|---|---|---|
| 24,885 | `0x8001BE1C` | 6 | 35 cells at x ∈ {24,72,…,312}, y ∈ {24,…,216} | a 5-row x 7-column BACKDROP GRID, 711 frames x 35 cells |
| 4,644 | `0x8001BD6C` | 8 | (128,212), (160,212), (192,212) | a three-item row along the bottom — a button legend |
| 711 each | `0x8001BCB4/B8/BC/C0` | 5 | — | four more, once per backdrop frame |
| 644 | `0x8001BD00` | 5 | — | — |
| 27 | — | — | — | drawn in scene 2 (Title); everything else is scene 3 (Field) |

The 35-cell grid at that exact pitch is the memory-card menu's backdrop as `card_menu.h` describes
it — "a 5x7 grid of menu template 238/239 at (24 + 48*col, 24 + 48*row)". So the card menu's own
chrome is being dropped on 711 frames of this route even though `CardMenu` has a scope, which means
those frames reach the grid by a path that is not under `FUN_8018FBCC`.

## Why it matters

Every one of these is a piece of chrome the guest linked into its ordering table and the product
never draws. The user reported the save menu; this is the same defect class, at 12x the count, on
the same route they were looking at.

## ROOT CAUSE of the biggest share (measured 2026-09-19): CardMenu was never installed

The 24,885 backdrop cells, and the 711 + 644 + 50 from three sibling page drawers, are the card
menu's own chrome — and `CardMenu`'s scope was never raised for any of it, because **the producer
had never run once**. Its override was declared with `declareOverride` at `0x8018FBCC`, an AREA-slot
address, and `bindResident` installs nothing outside the resident text range. The `cardmenu` channel
was silent for all 2,400 frames.

Declared with `declareOverlayOverride("CRD", ...)` it installs, and the card menu draws 52-72 items
per frame where it drew none. **Drops on this route: 33,805 -> 4,671.**

Verified against the committed reference `docs/reference/issues/save-browser-oracle.png`: the
backdrop motif, the "Save" title badge, the four-glyph legend row and the slot panels are all
present and placed as the reference has them, where `save-browser-pcrender.png` (the committed
"before") shows none of them. The live console leg cannot reach this screen — it fails strict
alignment at the memory-card access — so the committed reference is the independent evidence here,
not a live comparison.

That declaration bug is not confined to this page. 42 of 254 declarations are in the same state:
issue 0015.

## What remains: 4,671 drops, and NOT all of them are defects

`ui_group_capture.h` says outright that some unscoped calls "belong to independent producers such as
the field HUD and dialog box" — that is, chrome the product draws natively from game state, where
ignoring the guest's group emission is correct. The report cannot tell those apart, so the remaining
count is an upper bound on the defect, not the defect.

| count | caller | placement | status |
|---|---|---|---|
| 4,644 | `0x8007E988` | (128,212), (160,212), (192,212) bucket 8, FIELD | unattributed — check against the picture before treating as missing |
| 27 | various | scene 2 (Title) | unattributed |

## Next step

1. Find which caller emits the 35-cell grid on those 711 frames and whether `CardMenu`'s scope
   should cover it or a second page owns it. `uigroup` prints the template, placement and bucket;
   the `ra`-style caller census that localised 0013 (`reportTextEmit` in `game/ui/font.cpp`) is the
   shape to reuse on the sprite leaf.
2. Do the same for the bottom-row legend at y=212.
3. Do NOT "fix" this by making `route` draw unclaimed groups inline. Which page owns a group decides
   its layer and its paint order; drawing it under no page is how the card menu's backdrop painted
   over its own slot rows (`ui_group_capture.h`, the ONE LIST note).

## Related

Issue 0013 — same mechanism, the save prompt, fixed.
