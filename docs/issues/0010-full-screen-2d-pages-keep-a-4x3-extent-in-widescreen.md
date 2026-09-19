---
id: 10
title: Full-screen 2D pages keep a 4:3 extent in widescreen, so the options page has black pillars
status: open
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

## Mechanism (determined 2026-09-19, not guessed)

The framework already knows how to fill a widened screen with a background, and the page is missing
exactly one input to it.

`rq_2d_xform` (psxport `runtime/psx/render_queue.cpp`) resolves every 2D submission to one of two
transforms: `stretch` (spread across the wide framebuffer, `num=ww den=native_w`) or `shift` (centre
by the margin). A page that is centred rather than stretched is what produces the black pillars, and
the selection happens upstream, in the OT walk:

    int fill = !is3d && (node_is_bg(s_cur_node) || fade_full);   // gpu_native.cpp:1420

`node_is_bg` is **provenance, not coverage**, and deliberately so — psxport #38 records that using
the screen-coverage heuristic here bled a mis-tagged backdrop to the widescreen edges. It answers
true only for OT nodes inside a span that some drawer REGISTERED this frame through
`gpu_bg_range_add(core, lo, hi)`, and today the only caller is the field's own background override
(`submit.cpp ov_bg_tilemap`), which draws the sky/sea tilemap.

The options page's background is drawn by ordinary guest menu code. Nothing registers its span, so
`node_is_bg` is false, `fill` is 0, and `rq_2d_xform` centres it. That is the whole cause.

## Next step

RE the guest drawer that fills the options-page background, to learn which OT node or packet-pool
span its quads occupy, then have Tomba! 2 register that span with `gpu_bg_range_add` the way the
field backdrop does. No new framework mechanism is needed and none should be added: the stretch path
already exists and is already tested (`tests/test_rq_widen_2d.cpp`).

Two things must be established by that RE rather than assumed: whether one drawer serves all the
full-screen pages (title options, in-game options, and the still-uncaptured save/memory-card, Screen
adjust and Controls pages), and whether registering the span widens ONLY the background fill and
leaves the authored text and cursor at their authored positions. Widening the text with it would be
the stretch this issue forbids.
