---
id: 111
title: Guest render paths at the item menu: gte is missing 58% of the picture, psx draws NOTHING
status: todo
labels: [render, oracle, bug]
created: 2026-08-20
updated: 2026-08-20
---

USER 2026-08-20, live windowed run: 'the menu is too dark here, PC renderer is correct'. Their session was on PSXPORT_RENDER_PATH=gte (cvars confirmed it, runtime layer). Evidence: scratch/screenshots/live/menu_dark_now.png.

MEASURED HEADLESS, all three paths, same replay, same frame — replays/bugs/ingame-item-menu.pad at f1120 (PSXPORT_NATIVE_FRAMES=1160 PSXPORT_PRESENT_SHOT_AT=1120 PSXPORT_RENDER_PATH=<p>). Shots: scratch/screenshots/menu_{native,gte,psx}.png.

    path     mean RGB                non-black
    native   (76.14, 70.35, 42.90)     89.4%
    gte      (41.92, 35.43, 12.36)     37.1%
    psx      ( 0.00,  0.00,  0.00)      0.0%

FINDING 1 — 'too dark' is mostly 'MISSING', not 'dimmed'. 58.3% of the frame is lit on native and BLACK on gte. Over the 31.1% of the frame lit on BOTH paths, gte is at 0.665 of native's brightness, and the per-channel ratios are NOT uniform:
    R 0.7373   G 0.7131   B 0.4384
A flat texture-modulation error (treating GP0 colour 0x80 as 128/255 = 0.502 instead of 1.0) would halve all three channels EQUALLY, so that is NOT what this is — the hypothesis was tested and refuted by the numbers. Blue is hit roughly twice as hard as red/green, which points at colour-depth / CLUT handling or a wrong texture format rather than a modulation constant. Do not chase the modulation theory; it is already excluded.

FINDING 2, and it is the bigger one — psx_render draws LITERALLY NOTHING at this frame. 0.0% non-black, a fully black 960x720 frame. Not dark: empty.

FINDING 3 — A STALE CLAIM, now falsified. replays/README.md says of this recording: 'the in-game item/pause menu at frame 1120 ... Pixel-exact against psx_render at f1120, so it doubles as the #21 no-regression gate.' That cannot be true of a psx leg that renders an all-black frame. Either the claim rotted or the gate has been silently passing against nothing. Anything that cited this as a passing gate needs re-checking — grep for who relied on it.

RELATION TO #110 (wire beetle's vendored GPU as the real oracle): this card IS the case that motivates it, and it doubles as #110's acceptance test. The guest paths here are OUR rasterizer, not a reference. When beetle's GPU is wired, re-run this exact three-way capture:
  * if beetle renders the menu at native's brightness and coverage, then gte/psx were wrong all along and #110 fixed it;
  * if beetle also comes out dark or empty, the fault is in the guest's own packets and lives upstream of the rasterizer.
Either answer is decisive, which is what makes this a good gate — and it satisfies the 'an instrument is trusted only once it has shown the other answer' rule for the new backend.

REPRO (now reliable — it was NOT before psxport 088c4722, which stopped the headless frame cap from silently truncating a pad replay at 120 frames):
  PSXPORT_NOAUDIO=1 PSXPORT_NO_FMV=1 PSXPORT_NOPACE=1 PSXPORT_RENDER_PATH=<native|gte|psx> \
  PSXPORT_NATIVE_FRAMES=1160 PSXPORT_PRESENT_SHOT_AT=1120 \
  PSXPORT_PAD_RESUME=replays/bugs/ingame-item-menu.pad ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE

SUPERSEDES the open question from 2026-08-19 about whether to delete the gte path. The answer is now measured rather than argued: gte is not empty (it draws 37.1% of the frame here and 76,786 non-black px at free-roam), it is BROKEN — and psx is the one that draws nothing at this frame. Deleting gte would have hidden a rasterizer bug that psx has too.

**2026-08-20:** 2026-08-20 — THE THREE-WAY CAPTURE IS ANSWERED, and it is the option this card did not list.

#110's oracle is now calibrated and trusted (feed proven complete: 368 prims ours = 368 dispatched by beetle, every loss channel zero; positive control 1,692 PASS / 0 FAIL). So its answer is readable.

MEASURED at f1120 on PSXPORT_RENDER_PATH=psx, inside the display rect the guest itself declares (320x240 @ (0,0), from GP1(0x05)):

    ours     non-black 69101/76800 (90.0%)   mean RGB (74.29, 68.78, 41.94)
    beetle   non-black 69101/76800 (90.0%)   mean RGB (74.29, 68.78, 41.94)
    native (this card, presented frame)      mean RGB (76.14, 70.35, 42.90)  89.4%

Ours and beetle are IDENTICAL — 0 differing pixels across the whole 1024x512 VRAM, not just the display rect. And the picture psx_render puts in VRAM matches the NATIVE path's picture to within 2.4% per channel.

FINDING 2 OF THIS CARD IS REFUTED. 'psx_render draws LITERALLY NOTHING at this frame' is false. psx_render draws the menu, correctly, at native brightness, and byte-for-byte the same as real PSX hardware given the same command stream. The frame is not empty and it is not dark.

THE FAULT IS DOWNSTREAM OF RASTERIZATION. VRAM is right; the presented 960x720 frame is all black. So the loss is in presentation/scanout — what reads VRAM and puts it on screen on this path — not in the rasterizer and not in the guest's packets. Neither branch this card predicted was correct: beetle did not 'fix' the picture (ours was already identical to it), and the guest packets are not at fault either.

WHY THE EARLIER READING WAS WRONG: 'mean RGB of the presented frame' cannot tell 'nothing was drawn' apart from 'something was drawn and then not presented'. Those need different evidence and only VRAM separates them.

NEXT, and it is now a narrow search rather than a rasterizer hunt: find where the psx path's present drops a VRAM that is known-good. Start at GpuState::frame_finalize / gpu_present_ex and what psx_render passes to the presenter.

STILL OPEN, and NOT answered by this: the gte path. gte was measured at (41.92,35.43,12.36) / 37.1% with per-channel ratios R 0.74 / G 0.71 / B 0.44. Nothing above touches it — gte and psx are separate paths and psx being correct says nothing about gte. Run the same VRAM-vs-beetle comparison on PSXPORT_RENDER_PATH=gte before assuming its fault is downstream too.

REPRO for the VRAM measurement:
  PSXPORT_NOAUDIO=1 PSXPORT_NO_FMV=1 PSXPORT_NOPACE=1 PSXPORT_RENDER_PATH=psx \
  PSXPORT_GPU_BEETLE=1 PSXPORT_GPU_BEETLE_DUMP=1120 PSXPORT_DEBUG=gpubeetle,gpu \
  PSXPORT_PAD_REPLAY=replays/bugs/ingame-item-menu.pad ./scratch/bin/tomba2_port
then measure the 320x240 @ (0,0) rect of scratch/screenshots/{ours,beetle}_vram_f1120.ppm.

**2026-08-20:** 2026-08-20 — ROOT CAUSE FOUND AND FIXED (psxport 4393b5aa). The psx path presented an all-black frame because render_geom CLEARED AWAY the picture our own software rasterizer had just drawn.

THE THIRD SITE BLIND ON RenderPath::Psx, and the first two were already fixed and documented in gpu_vk_present_policy.h. On that path the software rasterizer draws the whole frame into s_vram, tees NO VK geometry and marks NOTHING dirty — so every input that asks 'did anything change' answers no:
  * the present DECISION was fixed for it (swRasterIsPicture)                 <- done 2026-08-11
  * the dirty list was fixed for it (s_dirty.markAll())                       <- done 2026-08-11
  * render_geom's clear-to-black was NOT                                      <- this
It consulted GameConfig::preserveVramBackdrop, which is the port's statement about whether the GUEST's VRAM is picture under the NATIVE renderer. Tomba!2 answers 0 and is RIGHT to: its native producers own the frame, so leftover guest VRAM is stale. But render_geom's batch is PERMANENTLY empty on the software path, so 'total == 0' there says nothing about whether a picture exists — and the clear wiped the s_vram upload_vram had uploaded three lines earlier.

MEASURED, before -> after, same capture (PSXPORT_PRESENT_SHOT_AT=1120, path=psx):
    presented frame non-black   0/691,200 (0.00%)  ->  272,604/691,200 (39.44%)
The present DECISION was already correct throughout: presentskip reported rebuild_vram=1160, reuse_last=1. And vramup reported regions=1 all=1 — the full VRAM really was uploaded. The clear ran after both.

FINDING 2 OF THIS CARD IS REFUTED, and 'the guest paths are broken rasterizers' with it. psx_render draws the menu correctly, at native brightness, byte-for-byte identical to the beetle GPU oracle. Nothing was ever wrong with the rasterizer at this frame.

A CORRECTION TO MY OWN FIRST READING OF THE FIXED FRAME: comparing 'lit' pixels between VRAM and screen with an any(rgb) test reported 43,502 pixels 'lost in presentation' in a central rectangle. That was a THRESHOLD ARTEFACT, not a finding — the item panel is a very dark non-zero grey (0x181818) in VRAM, so 'lit' counted it. The picture is substantially correct; see scratch/screenshots/psx_vram_vs_present_f1120.png (VRAM | screen) and psx_vs_native_present_f1120.png (psx | native).

TWO REAL DELTAS REMAIN, both psx-vs-native at this frame, and neither is the rasterizer:
  1. DARK VALUES CRUSHED TO BLACK. VRAM holds 0x181818 in the item panel; the psx present shows 0x000000 there while the native present shows the dark grey. A colour-conversion or fade issue in the s_vram_tex sampling path, not in what was rasterized.
  2. Slight vertical layout difference between the two presented frames (the bottom help panel sits higher on psx). Row-luminance correlation against VRAM is best at dy=0, so this is NOT a global offset — it needs its own measurement. Note the two captures compared were one frame apart in labelling (the pre-fix build labelled the dump one frame late), so RE-CAPTURE both on the current build before treating this as real.

STILL UNTOUCHED BY ALL OF THIS: the gte path. Its measured (41.92, 35.43, 12.36) / 37.1% is now numerically almost identical to what psx presents AFTER the fix (41.88, 35.53, 12.43 / 39.4%), which is worth knowing but is not evidence about gte — run the VRAM-vs-oracle comparison on PSXPORT_RENDER_PATH=gte before concluding anything.

TOOLING: tools/vram_oracle.py <replay> <frame> [--path psx|gte|native] [--selftest] does this comparison in one command and REFUSES (exit 2) rather than reporting a difference measured on a lossy feed or a run that never reached the frame.
