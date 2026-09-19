---
id: 10
title: Full-screen 2D pages keep a 4:3 extent in widescreen, so the options page has black pillars
status: resolved
symptom: at aspect=1 every full-screen 2D page measured so far is drawn 320 wide inside a 428-wide target, so it fills the height and 74.8% of the width; the looks-right widescreen check passes them all because the PNG "differs from 4:3", which pure rescaling also satisfies
state_items: S005
tags: tomba2,widescreen,2d,ui,instrument
created: 2026-09-19
updated: 2026-09-19
---

## Finding

Measured 2026-09-19 on `build/ci/bin/tomba2_port` at psxport `98eb3b02`, replay
`replays/bugs/title-options-page.pad` frame 1110, both legs captured by
`external/psxport/tools/port/looks_right.py` into a 960x720 sink. Bounding box of everything drawn
(non-black pixels):

| leg | drawn | position | ratio |
|---|---|---|---|
| 4:3 | 960x720 | x[0..959] y[0..719] | 1.333 |
| 16:9 | 718x538 | x[121..838] y[91..628] | **1.335** |

The widescreen picture draws its content at a 4:3 ratio inside a 16:9 target. It fills the full
height of the widened picture (538 of the 540-pixel letterboxed band) and 718/960 = 74.8% of its
width — which is exactly 320/428, the un-widened native width over the widened render width. The
page gains **zero** horizontal coverage; it is the same 320-wide page, scaled down because the
target around it got wider.

On a real 16:9 display that is black pillars either side of the options page. In the capture the
letterbox hides it, which is why it survived until the extent was measured rather than looked at.

## Why the gate passed it

`looks_right.py`'s `widescreen` check asserts the widened PNG DIFFERS from the 4:3 PNG. Rescaling
the whole picture satisfies that, so the check cannot distinguish "rendered additional world" from
"rendered the same content smaller". Its own docstring names this exact failure — "widescreen worked
but its 2D layers kept a 4:3 extent" — and the check it grew was not strong enough to catch it. A
drawn-extent comparison is the discriminator: the ratio must move toward the target aspect, not stay
at 1.333.

## Every page measured, and the cross-title check

Re-measured with psxport `a1537b73`'s `coverage` measurement over every paired capture in both
titles (15 pairs; drawn aspect, 4:3 leg -> 16:9 leg):

| scene | 4:3 | 16:9 | |
|---|---|---|---|
| Tomba! 2 title options page | 1.333 | 1.335 | no gain |
| Tomba! 2 in-game options page | 1.333 | 1.335 | no gain |
| Tomba! 2 item menu | 1.420 | 1.420 | no gain |
| Tomba! 2 3D world scene | 1.333 | 1.784 | wider |
| Spyro 3D scenes (x8) | 1.429 / 2.286 | 1.912 / 3.054 | wider |
| Spyro level-intro card | 1.992 | 2.000 | no gain — the scene is black, so there was nothing to gain |
| Spyro `secondary` f399 | 1.466 | 1.470 | no gain — not yet identified |

So this is not one page. Every Tomba! 2 2D page measured keeps its 4:3 extent while every 3D scene
in both titles genuinely widens. Spyro's own `secondary` f399 is an unidentified no-gain scene and
is NOT claimed here as the same defect.

## Scope, and what is NOT this bug

The 3D world paths are not affected: S005's seaside, hut, cliff/village and water-pump captures each
show genuinely more world at the sides, and the `[wide] native picture:` line reports
`render_width=428` against `native_width=320` for all of them.

The in-game item menu (`replays/bugs/ingame-item-menu.pad` f1110) measures 954x672 at 4:3 and 713x502
at 16:9 — ratio 1.420 in BOTH legs, so it behaves the same way. That one is arguably correct as it
stands: it is a bordered window with authored edges, and centring a window is a normal widescreen
choice. The options pages are not windows — they are full-screen background fills, and a full-screen
fill that stops at 4:3 is a hole in the picture.

Interpolation is not implicated. With `PSXPORT_FPS60=1` the real frames of the item-menu route are
byte-identical to the 4:3 leg's at both f1000 (3D) and f1110 (menu) — 0 of 691,200 pixels differ —
while that same run emitted 615,338 interpolated prims over 1,091 extra presents. Interpolation adds
frames without touching the authored ones.

## Not to be fixed by stretching

Widening this must not scale the page horizontally or stretch the final image. The background fill
is the element that should extend to the wide extent; the authored text and cursor stay at their
authored positions relative to the page's centre.

## Mechanism — the first account was WRONG in two load-bearing ways

This issue originally pinned the cause on the framework OT walk: `fill = !is3d && (node_is_bg(...) ||
fade_full)` in `gpu_native.cpp`, with `node_is_bg` answering true only for spans a drawer registered
through `gpu_bg_range_add`, and the next step was to have Tomba! 2 register the options-page span
"the way the field backdrop does". Both halves are false, and acting on them would have produced
nothing.

**`gpu_bg_range_add` has no callers at all.** Not in psxport, not in any game in the workspace. The
named exemplar — "the field's own background override (`submit.cpp ov_bg_tilemap`)" — does not
exist; `gpu_native.cpp` itself calls that packet-span provenance "dead" and records that the texpage
path (`gpu_bg_texpage_set`) replaced it. So `node_is_bg` can never return 1 today, and there was no
working caller to copy.

**That code does not run for this page anyway.** Measured 2026-09-19: the product logs
`render path = native — geometry from PC-NATIVE producers`. `PSXPORT_PRIMDUMP=1110` was set for a
whole run over `replays/bugs/title-options-page.pad` and the config audit reported
`UNKNOWN knob PSXPORT_PRIMDUMP was set for this whole run and NOTHING ever read it` — the guest OT
walk never executes for these pages, because the title's own native producers draw them. The `fill`
selection, `node_is_bg`, and `rq_2d_xform`'s stretch branch are all downstream of a path this page
never takes.

The page is drawn by `Render::optionsBackdrop` (options family) and `Render::renderCardBrowser`
(memory-card page), and the pause/item menu by `PauseMenu`. The real cause is simply that each
authored its background at the guest's 320-wide extent and nothing owned what the widened canvas
shows beside it.

## What the margin actually needed, and why it is not a stretch

The page background is not texture art — it is ONE untextured Gouraud quad. Guest `FUN_8007FC24`
draws (0,0)-(320,240) with per-vertex blue TL/TR/BL = `0x46` and BR = `0x10`; the pause/item menu's
is the guest's GP0 `0x60` tile, uniform black. Both are therefore extendable deterministically.

Extending them by widening the quad would be the stretch this issue forbids: spreading the gradient
across 428 columns moves the authored bottom-right darkening to a different screen position and
changes the page INSIDE the 4:3 region. The answer is a CLAMP-CONTINUATION — each margin band
carries the colour the authored gradient already has at the edge it touches, held constant outward.
The left edge is (TL, BL) and the right edge is (TR, BR), so the left band is flat `0x46` and the
right band carries the same `0x46` -> `0x10` fall the page's right edge has. Nothing is invented: the
value at a margin column is the value the authored gradient defines at the boundary.

## Fix

`game/render/page_gradient.{h,cpp}` owns each authored page gradient ONCE — `PageGradient::optionsPage()`
for `FUN_8007FC24` and `PageGradient::pauseMenu()` for the menu's black tile — plus `pageMarginBands`,
the band rule as pure arithmetic (no `Core`, no queue, no globals) so it is tested hermetically the
way the framework tests `rq_2d_xform`. `game/render/page_backdrop.{h,cpp}` draws them:
`PageBackdrop::pushAuthored` for the page itself and `PageBackdrop::pushMargins` for the margins.

This also removed a duplication the previous fix left standing: `render_options.cpp` and
`card_browser.cpp` each carried their own copy of the same 22-argument quad push AND their own copy
of the same corner colours, for the same guest function. The superseded `wide_page_fill.{h,cpp}` is
deleted — its black canvas quad is gone, and black now appears only where a page is authored black,
as a consequence of that page's own colour rather than a fill chosen for every page.

## Evidence

Measured on `build/ci/bin/tomba2_port` at psxport `a1537b73`, via
`external/psxport/tools/port/looks_right.py`, with `PSXPORT_AUTO_SKIP=1`. The pre-change build was
rebuilt from a stash to produce the negative control rather than quoting the earlier numbers:

| scene | before | after |
|---|---|---|
| title options page (`title-options-page.pad` f1110) | 1.333 -> 1.335 NO GAIN | 1.333 -> **1.784** WIDER |
| in-game options page (`ingame-options-page.pad` f1160) | 1.333 -> 1.335 NO GAIN | 1.333 -> **1.784** WIDER |
| item menu (`ingame-item-menu.pad` f1120) | 1.420 -> 1.420 | 1.420 -> 1.420 |

16:9 is 428/240 = 1.783, so the two options pages now draw to the full canvas. The item menu's
reading is unchanged BY DESIGN and is not a remaining defect: its page is authored black, its margins
are therefore black, and `drawn_extent` measures non-black pixels — the tool's own documented blind
spot. Its capture shows a bordered parchment window centred on black with no field showing through,
which this issue already recorded as the correct treatment for a window.

- **Seam continuity, measured not eyeballed.** On the 16:9 title-options capture the maximum
  adjacent-column mean-colour delta is **0.00** at the left seam (x=121) and **0.18** at the right
  (x=839), against **13.32** elsewhere in the picture (the text). The bands abut the centred page
  with no visible join.
- **4:3 is byte-identical.** `sha256 f70be71b2059408c821744b0abbb998d24331f40cf25221a9c7ee50d9fa0f5ef`
  for the 4:3 leg of both the pre-change and post-change builds. The margin rule returns zero bands
  at 4:3, so no extra prim enters a 4:3 frame.
- **Oracle.** `tools/oracle_compare.py --frame-step 1` with `PSXPORT_FPS60=1` and a settings file
  carrying `aspect=1`: **405/405 checkpoints, 3,240 decisive range comparisons, 0 divergences**,
  `complete: true`. `--selftest` seeded a byte at `0x800E7EAC` and the comparator DETECTED it, so the
  zero is a measurement rather than a silent pass.
- **The rule's own test shows both answers.** `tests/test_page_gradient.cpp` (ctest
  `tomba_page_gradient`) asserts the 4:3 negative first, the band geometry, the continuation colours,
  and the impossible-input refusals. Seeding the exact defect it exists to prevent — the right band
  taking the LEFT edge's colour — made it fail with
  `FAIL: the right band's bottom continues BR — got (0,0,70) want (0,0,16)`; restoring gave a file
  byte-identical to the pre-seed original.
- **Repository gate.** `tools/verify_ci.py`: 26/26 ctest cases, C++ policy over 417 first-party
  files, execution boundary clean, psxport pin `a1537b73` matching the build.

## Still open

The Screen-adjust page (page 3) and the Controls page (page 4) are not captured by any replay, so
they are unmeasured. Screen adjust deliberately draws NO backdrop and composites over the live title
picture, so it is expected to need no margins; Controls uses the same `FUN_8007FC24` backdrop and
should already be covered by `Render::optionsBackdrop`. Neither is claimed here.
