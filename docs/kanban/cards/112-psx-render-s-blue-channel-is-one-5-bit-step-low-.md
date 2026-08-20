---
id: 112
title: psx_render's blue channel is one 5-bit step low vs real hardware on gradient fills
status: done
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

**2026-08-20:** 2026-08-20 — SCOPE CORRECTED, and a SECOND difference separated out as #113.

'Dither is refuted' remains true FOR THIS SCREEN and is now much better supported, because the same test came out POSITIVE on another one. On the in-game START page (f1090 of the same replay, a full 3D scene) the per-cell differ rate over a 76,800-pixel denominator ranges 6.9% to 56.9% — a factor of eight, the 4x4 ordered matrix showing plainly. Here it is flat at 2.7-3.4%. Same test, same denominator method, opposite answers: that is what makes both readings trustworthy rather than one of them being an artefact of the test.

Full four-screen picture, psx path, every one with the feed proven complete first:

  screen                        prims          differing        signature
  item menu        f1120    368 = 368        0  (0.00%)   pixel-identical
  START page       f1090    971 = 971   27,561  (5.26%)   DITHER, all channels -> #113
  options page     f1160     70 =  70    2,309  (0.44%)   blue only, no 4x4 -> THIS CARD
  title options    f1027      5 =   5        0  (0.00%)   pixel-identical

Visual: scratch/screenshots/oracle_menus.png. This card's difference renders as blue DIAGONAL CONTOUR BANDS — the gradient's own iso-value lines, which is what a quantisation-boundary rounding difference looks like and is visibly nothing like #113's dense speckle.

So the two are separate bugs with separate causes and must not be fixed together.

**2026-08-20:** 2026-08-20 — UNCHANGED BY #113's FIX, which is itself informative. The dither fix (psxport 05ce4045) took the START page from 27,561 differing pixels to 5,891 and left this card's options page at exactly 2,309. Two separate bugs, confirmed by a change that moved one and not the other.

A NEW LEAD, and it may merge this card with #113's residual. After the dither fix the START page still differs on 5,534 px that are EXACTLY one 5-bit step and UNIFORM across the 4x4 cells — the same signature as this card, except spread over all three channels (R 2,109 / G 3,634 / B 1,133) instead of blue only. This card's screen is a blue-only gradient, so 'blue only' there and 'all channels' here are consistent with ONE underlying interpolator-rounding bug showing on content with different colour content.

TEST THAT BEFORE FIXING EITHER: if it is one bug, a fix here must also drop the START page's 5,534. If it does not, they are two and this card's scope was right. That is a cheap check and it decides whether to fix once or twice — python3 tools/vram_oracle.py replays/bugs/ingame-options-page.pad 1090 --path psx is the counter-measurement.

STILL SEPARATE FROM BOTH: 357 px on the START page differ by MORE than one step (up to +-184) and were unaffected by the dither fix. A third mechanism, uncharacterised.

**2026-08-20:** 2026-08-20 — FIXED, and it was ONE bug with #113's residual, exactly as the counter-measurement on this card predicted. psxport 2a0820a5.

ROOT CAUSE: beetle seeds every interpolant with a half-LSB bias, so its DDA ROUNDS TO NEAREST:
    gpu_polygon.c:904   ig.u = (COORD_MF_INT(u) + (1 << (COORD_FBS - 1 - shift))) << ...
    gpu_polygon.c:945   ig.r = (COORD_MF_INT(r) + (1 << (COORD_FBS - 1)))         << ...
gpu_native.cpp already replicated that for U and V — its own comment says so — and did NOT for the interpolated COLOUR, which used a plain integer divide. Integer division truncates toward zero, so our colour was systematically LOW on every gouraud and every texture-modulated pixel.

HOW THE DIRECTION IDENTIFIED IT: after #113's dither fix the START page still differed on 5,534 px by exactly one 5-bit step, and 5,896 of the 6,031 per-channel differences (97.6%) were ours-LOW. A symmetric rounding error is ~50/50; a one-sided bias is truncation. On this card's blue-only gradient it was 100% ours-low in blue — same bug, and 'blue only' was simply the only channel with a gradient.

RESULT, four screens, psx path, feed proven complete on each:
                         before #113   after #113    after this fix
    f1090 START page       27,561        5,891           561      -98.0% overall
    f1160 options page      2,309        2,309            15      -99.4%
    f1120 item menu             0            0             0
    f1027 title options         0            0             0

THE COUNTER-MEASUREMENT WAS THE POINT. This card recorded that if #112 and #113's residual were one bug, a fix here MUST also drop the START page's 5,534 — and it did, to 561. Had it not, they were two and the scope was right. That decided fix-once vs fix-twice before any code was written.

WHAT IS LEFT, and it is small enough to be a separate question: 15 px on the options page and 561 on the START page. The START page's 357 large-delta pixels (up to +-184) are still in there and are still a third, uncharacterised mechanism — unchanged by both fixes.

TEST: tests/test_bary_round.cpp, hermetic. Asserts BOTH windings (the doubled area is signed, and floor() would round the wrong way for one of them, moving the bias rather than removing it) and the exact cases, so a fix that just adds +1 fails.
