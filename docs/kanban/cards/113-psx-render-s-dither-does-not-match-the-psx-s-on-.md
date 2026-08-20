---
id: 113
title: psx_render's dither does not match the PSX's on 3D scenes
status: todo
labels: []
created: 2026-08-20
updated: 2026-08-20
---

A SECOND, DISTINCT rasterizer difference found by the calibrated beetle oracle (#110), and it is much larger than #112's. The two must not be conflated: they have opposite signatures and almost certainly different causes.

MEASURED: replays/bugs/ingame-options-page.pad f1090, PSXPORT_RENDER_PATH=psx — the in-game START page (Options / Load data / Quit game over the live 3D scene).

    ours vs beetle, whole VRAM      27,561 / 524,288 differing  (5.26%)
    inside the 320x240 display rect 27,561
    outside it (textures / atlas)        0

FEED PROVEN COMPLETE: ours drew 971 prim(s), beetle dispatched 971. dropped 0, starved 0, unknown 0, null-func 0, fifo queued 0.

THE SIGNATURE IS DITHER, and unlike #112 the 4x4 test comes out POSITIVE. Per-cell differ rate over the full 76,800-pixel denominator (4,800 per cell):
    56.9%   6.9%  46.2%  27.8%
    43.9%  33.2%  54.4%  16.7%
    48.0%  27.9%  56.9%   7.1%
    55.7%  16.3%  42.4%  34.1%
A factor of EIGHT between the extreme cells. The PSX dithers with a 4x4 ordered matrix, and this is that matrix showing through. Compare #112's options page, where the same test came out flat at 2.7-3.4% and dither was correctly refuted — same test, opposite answer, which is what makes both readings trustworthy.

MAGNITUDE: 27,213 px (98.7%) differ by EXACTLY one 5-bit step; all three channels participate (R 13,068 / G 13,632 / B 11,531 px) with means near zero (+0.79 / -0.04 / +0.75), i.e. symmetric, no brightness bias. Visually the amplified diff is dense multi-coloured speckle over the whole 3D scene — scratch/screenshots/oracle_menus.png, row 2.

A THIRD EFFECT HIDES INSIDE THIS ONE, and is NOT dither: 348 px (1.3%) differ by MORE than one step, up to +-184. Their 4x4 buckets are uniform (14-27 per cell), so they are position-independent and therefore a different mechanism from the 27,213 above. Not yet characterised; do not fold them into the dither fix or into #112.

WHY THE OTHER THREE SCREENS DO NOT SHOW IT: the item menu (368 prims) and the title options page (5 prims) are 0.00% different, and the in-game options page differs only in blue with no 4x4 structure. This screen is the only one of the four drawing a full 3D scene, which is where dithered Gouraud/texture-blended fills actually occur. So the honest scope is 'dithered 3D content', not 'everything'.

VISIBILITY: one 5-bit step is invisible in isolation; whether this is worth fixing on its own is a judgement call. It matters mainly because it makes any FUTURE pixel comparison on a 3D scene noisy — 5% of the frame differing for a known benign reason will mask a real regression. Fixing it buys a clean baseline for every 3D comparison the oracle will be used for.

REPRO:  python3 tools/vram_oracle.py replays/bugs/ingame-options-page.pad 1090 --path psx

HOW IT WAS FOUND: the tool REFUSED this frame the first time (exit 2, 'FEED INCOMPLETE: ours drew 971 but beetle dispatched 975'). The refusal was right and the census was wrong — beetle dispatches each POLYLINE SEGMENT separately, so 2 polylines read as 6 line dispatches. Same trap as the quad-continuation counter, fixed the same way (PGC_LINE_CONT). Without the refusal this would have been read as a 5.26% rasterizer difference on a feed that was never verified.
