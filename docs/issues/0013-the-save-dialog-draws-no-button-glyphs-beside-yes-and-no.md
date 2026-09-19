---
id: 13
title: The save dialog draws no Cross/Circle button glyphs beside Yes and No
status: fixed
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

## ROOT CAUSE (measured 2026-09-19): the prompts were emitted, routed, and dropped

The glyphs are not a missing producer. They are a producer whose output had no owner.

The prompt is drawn by the resident guest function `FUN_800738B0`, reached 1,415 times on this
route (`textemit` census, `ra` of the three drawText calls). It emits:

| what | how |
|---|---|
| Cross prompt | `FUN_80033AFC(0x4000, 188, 88, 0)` -> template `0x80017B74` |
| Circle prompt | `FUN_80033AFC(0x2000, 188, 108, 0)` -> template `0x80017B70` |
| "Save?" / "Yes" / "No" | three `FUN_80079374` (drawText) calls |
| the dialog panel | `FUN_8005019C` |

`FUN_80033AFC` hands its template to the game-wide 2D sprite group leaf `FUN_8007E6DC` — the same
leaf, with the same pad-bit-to-template mapping, that the card menu's prompts use (`card_menu.h`).
That leaf is tapped by `ui_sprite.cpp`'s `ov_compose`, which routes every group to
`UiGroupCapture::route`. `route` files a group under whichever page scope is raised and **drops it
when none is**. No scope covered this prompt, and its one classified-scene fallback does not claim
it either: the prompt is drawn over the FIELD scene (`classifyScene()` = 3), not
`SaveContinueMenu`.

Measured, with the drop reported for the first time (`uigroup` channel, added in this change):
**both prompts routed and dropped 1,415 times each**, exactly as often as the dialog drew its three
words. The text survived only because the Font taps produce it independently — which is why the
picture showed the words and nothing else.

The magenta smear where the Cross belongs is the field showing through the transparent hole the
panel left for a glyph that never arrived.

## Why nothing caught it

`route` dropped in silence. It was the `if (found) report()` shape the standing rule names: a
branch that draws when a scope claims the item and says nothing at all when none does. 33,805
further groups were being dropped on this one route and no instrument in the tree could say so.
That diagnostic gap is fixed here too, and its first run immediately produced issue 0014.

## Fixed (2026-09-19)

`SavePrompt` (`game/ui/save_prompt.{h,cpp}`) raises a capture scope around `FUN_800738B0` and drains
it at RQ_OVERLAY, the same shape StartPage and CardMenu already use.

The scope sits on the DRAWER, not on the prompt's task `FUN_800739AC`. The task was tried first and
fired zero times. The reason is ownership: `FUN_800739AC` is **already native** —
`beh_scene_ui_trigger`, the per-object behaviour handler the field placement driver installs at
node+0x1c — so there is no guest body there for a scope wrapper to run around, and it is reached
through the node's behaviour pointer rather than by `jal` (confirmed: no `jal` and no static table
word for that address in 178,688 scanned). The native handler reaches the drawer by typed dispatch,
which is where the scope now sits.

### Evidence

| | before | after |
|---|---|---|
| prompts filed under a scope | 0 | 1,415 each |
| prompts DROPPED at (188,88)/(188,108) | 2,830 | **0** |
| items drawn per prompt frame | — | 8 (6 panels, 2 groups) |

Picture at route frame 1,545 against the Beetle console reference: the product now draws the blue
Cross beside "Yes" and the red Circle beside "No", in the reference's positions and colours.
`scratch/picture/save-card-pages-1545f.{native,console}.png`.

The whole-frame difference at that checkpoint is 58.63%, unchanged in character by this fix: it is
the native renderer's world against the console rasteriser's, which is issue 0012's subject, not
this one. The dialog box itself is what this issue was about and what the pictures agree on now.

`tools/verify_ci.py`: 26/26.
