---
id: 114
title: psx_render: ~561 px still differ from real hardware, incl. 357 by more than one LSB
status: todo
labels: []
created: 2026-08-20
updated: 2026-08-20
---

THE RESIDUAL after #112 (interpolator rounding) and #113 (dither) were fixed. Recorded so the remaining difference is not silently treated as noise, and so anyone reading a future comparison knows what the expected floor is.

MEASURED, psx path, feed proven complete on every frame:
    f1090 START page        561 / 524,288  (0.11%)   was 27,561 before #113
    f1160 options page       15 / 524,288  (0.00%)   was  2,309 before #112
    f1120 item menu           0
    f1027 title options       0

TWO POPULATIONS, and they were separable before the fixes landed, so they should be separated now too:
  * ~200 px of exactly one 5-bit step. Whatever is left of the rounding family. Small enough that it may be a genuinely different edge (a clamp boundary, a semi-transparency blend rounding) rather than the same cause.
  * 357 px differing by MORE than one step, up to +-184. UNCHANGED IN COUNT BY BOTH FIXES (it was ~348 before #113 and 357 after), which is the strongest evidence available that it is a third, independent mechanism. Large deltas mean a pixel is a different COLOUR, not a slightly different shade — a wrong texel, a wrong blend mode, or a coverage difference at an edge.

START WITH THE 357, not the 200: they are the ones a human could actually see, and their (x%4, y%4) buckets are uniform (14-27 per cell), so they are not dither and not positional. Next step is to locate them spatially and look at what primitive covers them — the pixtrace at PSXPORT_PIXTRACE=\"x,y\" dumps our side's per-pixel math for one coordinate, and beetle's value for the same pixel comes from the VRAM dump.

WHY THIS IS WORTH A CARD RATHER THAN A SHRUG: the oracle is now accurate enough that 0 differing pixels is the normal result on two of four screens. A known, named residual is what lets the next regression be spotted immediately; an unexplained 0.11% is what makes people stop reading the number.

REPRO: python3 tools/vram_oracle.py replays/bugs/ingame-options-page.pad 1090 --path psx

**2026-08-20:** 2026-08-20 — CHARACTERISED. One hypothesis REFUTED by its own null, and the real mechanism narrowed to UV sampling.

SHAPE OF THE 354 LARGE-DELTA PIXELS (f1090, >1 LSB, up to +-184):
  * 346 of 354 are ISOLATED — no neighbouring pixel also differs by more than one step. Not edges, not runs, not a shifted shape.
  * spatially spread but weighted to the lower half (y 192-239 holds 171 of 354, the grass/ground), and across every column band.

HYPOTHESIS REFUTED — 'beetle sampled the screen-space neighbouring pixel's texel'. Eyeballing the first few cases was suggestive: at (42,3) ours (16,32,16), beetle (8,0,0), and OUR LEFT NEIGHBOUR is exactly (8,0,0). Three of the first eight looked like that.

THE NULL KILLS IT. Testing 'does beetle's value equal one of our four neighbours' across all 354:
    L 11.9%   R 1.1%   U 4.2%   D 9.3%   none 73.4%     -> 26.6% have a matching neighbour
and the same test on 2,000 pixels where the two AGREE:
    a matching neighbour exists BY CHANCE in 60.9%
So a matching neighbour is LESS common among the differing pixels than among agreeing ones. The eyeballed cases were coincidence. This is exactly what a denominator is for, and without it this card would have carried a confident wrong cause.

WHAT IT ACTUALLY IS, from a pixel trace at (127,13) — ours (64,160,0), beetle (40,128,0):
    ONE primitive writes it: tex=4 shade=1 semi=0 dith=true, uv=(125,28), texel=02C9
    our own math is fully accounted for, with no slack:
        R: texel5 9 -> 72 * 118/128 = 66, dither -1 -> 65, >>3 = 8   (VRAM 8)
        G: texel5 22 -> 176 * 118/128 = 162, dither -1 -> 161, >>3 = 20 (VRAM 20)
    For beetle's (5,16,0) with the SAME modulation colour, the texel it sampled must have been about
    (5..6, 17..18, 0) where ours was (9, 22, 0) — a different, DARKER texel.
So: a UV SAMPLING difference. Not coverage (one prim, same pixel claimed), not blending (semi=0), not the modulation, not dither, and not a screen-space offset.

THE LIKELY MECHANISM, stated as a hypothesis and NOT yet tested: we evaluate UV exactly per pixel and round, while beetle runs a fixed-point DDA — it seeds u/v with a half-LSB bias (gpu_polygon.c:904) and then steps by per-pixel deltas whose divide is TRUNCATED (CalcIDeltas: `CALCIS(u,y) * (1 << COORD_FBS) / denom`). A truncated delta accumulates a small downward drift along a span, so beetle's u/v ends slightly BELOW the exact value, and at pixels sitting near a texel boundary it lands one texel earlier. That predicts beetle sampling a lower-u/lower-v texel, which is consistent with the darker texel above but is a single case.

HOW TO TEST IT CHEAPLY BEFORE COMMITTING TO ANY FIX: extend pixtrace to print the texpage/CLUT alongside uv (the reddbg probe already prints both, so the plumbing exists), then read the four UV-neighbours of (125,28) out of the VRAM dump and see whether beetle's implied texel is one of them, and consistently on the low side. If it is, matching hardware means replicating the DDA rather than evaluating exactly — a real piece of work, and worth doing only if the count justifies it.

SIZE CHECK BEFORE ANYONE STARTS: 354 px of 524,288 is 0.07%, isolated, on one screen. This is a KNOWN FLOOR, not a visible defect. Its value is that it is named — so the next comparison that reads 0.07% is recognised as clean and anything above it is a regression.
