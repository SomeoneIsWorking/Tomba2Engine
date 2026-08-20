---
id: 112
title: psx_render's blue channel is one 5-bit step low vs real hardware on gradient fills
status: todo
labels: []
created: 2026-08-20
updated: 2026-08-20
---

THE FIRST GENUINE RASTERIZER DIFFERENCE THE CALIBRATED BEETLE ORACLE HAS FOUND (#110). Small, precise, and worth having exactly because it is small: it is the difference that survived once the feed was proven complete.

MEASURED: replays/bugs/ingame-options-page.pad f1160, PSXPORT_RENDER_PATH=psx, the in-game Select Options page (a near-pure blue gradient background: display-rect mean RGB (2.96, 3.48, 57.84), 100% non-black).

    ours vs beetle, whole VRAM      2,309 / 524,288 differing  (0.44%)
    inside the 320x240 display rect 2,309
    outside it (textures / atlas)       0

THE SIGNATURE IS UNAMBIGUOUS. Across ALL 2,309 differing pixels:
    red   max |delta| = 0
    green max |delta| = 0
    blue      delta   = -8 on every single one
-8 in the .ppm encoding is <<3, i.e. EXACTLY ONE STEP of the 5-bit blue channel. Ours is one step LOW; beetle (real hardware) is one step higher. The most common pairs are the gradient's own steps: 56->64, 48->56, 40->48, 32->40, 24->32.

FEED PROVEN COMPLETE at that frame, which is what makes this readable as a rasterizer verdict at all:
    ours drew 70 prim(s), beetle dispatched 70
    words accepted 325, dropped 0 | starved 0 | unknown 0 | null-func 0 | fifo queued 0

DITHER IS REFUTED, by measurement rather than by argument. The PSX dithers with a 4x4 ordered matrix, so a dither mismatch must concentrate in particular cells of (x%4, y%4). It does not — with a proper denominator (all 74,307 pure-blue pixels in the rect, bucketed the same way) the per-cell differ RATE is flat:
    3.4% 2.9% 3.4% 3.1%
    3.2% 2.7% 3.4% 3.1%
    3.3% 2.8% 3.4% 2.9%
    3.1% 2.7% 3.3% 2.9%
A 4x4 pattern would show cells at ~0% and cells at ~100%. This is uniform, so whatever differs is per-pixel and value-dependent, not position-dependent.

SO THE HYPOTHESIS IS A ROUNDING DIFFERENCE IN THE GOURAUD BLUE INTERPOLATOR: ~3% of blue pixels land on a boundary where our fixed-point step truncates one LSB below where the hardware lands. Consistent with truncate-vs-round, or with a different accumulator width/step derivation. NOT YET CONFIRMED — the next step is to compare the two interpolators directly on a single known gradient primitive rather than to infer from the aggregate.

WHY IT ONLY SHOWS IN BLUE HERE: this screen's gradient is essentially blue-only (mean R 2.96, G 3.48, B 57.84), so red and green have no gradient to disagree about. Do NOT read this as 'a blue-specific bug' until a screen with a red or green gradient has been measured — the honest claim so far is 'the channel that has a gradient is the channel that differs'.

REPRO:  python3 tools/vram_oracle.py replays/bugs/ingame-options-page.pad 1160 --path psx

RELATION TO #111: unrelated and much smaller. #111's black screen was presentation clearing away a correct picture (fixed, psxport 6fc7358c). This is the rasterizer itself, off by one LSB on 0.44% of one screen.

**2026-08-20:** 2026-08-20 — SCOPE MEASURED across three menu screens, psx path, each with the feed proven complete first:

  replay / frame                                 prims (ours=beetle)   differing px
  ingame-item-menu.pad      f1120                     368 = 368         0 / 524,288   0.00%
  ingame-options-page.pad   f1160                      70 =  70     2,309 / 524,288   0.44%
  title-options-page.pad    f1027                       5 =   5         0 / 524,288   0.00%

So this is NOT a general rasterizer inaccuracy. Two of the three screens are PIXEL-IDENTICAL to real hardware, including the item menu's 368-primitive scene. Only the options page differs, and only in blue, and only by one 5-bit step.

WHAT SEPARATES THE OPTIONS PAGE FROM THE OTHER TWO: it is the only one of the three built on a large smooth GRADIENT (display-rect mean RGB (2.96, 3.48, 57.84), 100% non-black, and the differing values are exactly the gradient's own steps 24/32/40/48/56/64). The title options page is a bright flat-ish screen (mean (138.38, 122.45, 114.39), 90.7% non-black) and matches exactly; the item menu is flat panels and text and matches exactly.

That is consistent with the interpolator-rounding hypothesis and inconsistent with anything global: a wrong colour conversion, a wrong blend or a wrong texture format would show on all three.
