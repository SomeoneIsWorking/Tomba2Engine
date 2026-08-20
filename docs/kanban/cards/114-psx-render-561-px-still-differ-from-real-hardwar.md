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
