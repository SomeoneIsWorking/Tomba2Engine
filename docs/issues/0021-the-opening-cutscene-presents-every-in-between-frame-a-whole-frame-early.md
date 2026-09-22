---
id: 21
title: The screen fade is a present-time composite with no endpoints, so during a fade the whole picture is a frame early
status: fixed
symptom: across the opening narration the in-between present was byte-identical to the NEXT real frame and did not move when the interpolation factor was forced to 0, on content whose two real endpoints were 0.67 apart
state_items: S005, S006
tags: tomba2,fps60,cutscene,interpolation,fade
created: 2026-09-22
updated: 2026-09-22
---

## Finding

Run the same replay twice, once at the product's own interpolation factor and once with
`PSXPORT_FPS60_TFORCE=0`, and every pixel whose two real endpoints differ has to say which endpoint
it landed on. Content that is interpolated sits at the PREVIOUS endpoint when the factor is forced
to 0. Content that is not sits at the NEXT endpoint whatever the factor is — a whole frame early.

Measured 2026-09-22, `replays/bugs/walk-dust-puff.pad`, `aspect=1 fps60=1`, 428x240, with all 307
shared real frames byte-identical between the two runs, so the factor was the only change:

| capture | triples | responded to t | did NOT respond | wholly unresponsive, continuous |
|---|---|---|---|---|
| fences 1..307 (boot, opening narration) — before | 247 | 64.66% | 30.21% | **66 triples** |
| fences 1..307 — after the fix | 247 | **93.37%** | 4.68% | **0** |
| fences 600..900 (gameplay), before and after | 299 | 96.35% | 2.19% | 0 (unchanged) |

The 66 continuous triples covered 3,430,224 changed pixels, a median of 49,629 per triple against
102,720 in a frame — half the screen, not a handful of pixels.

## The title is not what was wrong, and the first framing of this issue was

This was filed as "the opening cutscene does not interpolate its geometry". That was wrong, and two
counts refuted it before any fix was attempted:

- **Reconstruction was running.** `tier1=` averaged 233.7 prims per in-between present over those
  fences, against 0 on 33 of 35 boot frames. The scene was being rebuilt.
- **The objects were lerping.** A new per-present census of `Fps60::projObj` splits its three
  outcomes. Over the opening: 1,872 object resolutions, **98.1% lerped** from a real previous
  endpoint, 1.9% with no previous entry, 0 uncaptured. Gameplay reads 99.9%.

What was actually changing between those real frames was not geometry at all. Classifying the
per-pixel deltas between consecutive real frames:

| stretch | changed px | distinct deltas | share that is ONE uniform level step |
|---|---|---|---|
| opening, fences 55..120 | 4,864 – 102,355 | 7 – 262 | **89% – 100%** |
| gameplay, fences 170..300 | 71,276 – 86,162 | 8,427 – 12,116 | 22% – 35% |

Nothing was moving. A global level was, by 8 of 255 per logic frame — the `fadetrace` channel shows
the ramp directly: `SUBTRACTIVE 0xD8D8D8 -> 0xD0D0D0 -> 0xC8C8C8`.

A test that did NOT survive its control is worth recording: "every delta is a multiple of 8" looked
like a fade signature and scored 0 non-multiples over 1.96M changed pixels — and scored exactly the
same on gameplay frames, because PSX RGB555 quantises every colour to 8 steps. The discriminator
that does separate the two classes is the count of DISTINCT deltas, above.

## Cause

A screen fade is a **present-time composite**, not a queue item. Nothing in the captured frame
carries it and no producer reconstructs it: `gpu_vk.cpp` called `game_render_fade_state`, which
reads the title's CURRENT frame state, and **both presents of a logic frame called it**. So the
in-between present composited the next real frame's fade level. During a fade that is every pixel.

Every other lerped input already had endpoints — `mCamCur/mCamPrev`, `mBgCur/mBgPrev`,
`mObjCur/mObjPrev`. The fade had none.

## Fix

- `psxport::fade::resolve(prev, cur, havePrev, t)` — a new owner in
  `runtime/psx/fade_interpolation.{h,cpp}` for how two fade endpoints resolve at a factor. Returns
  `cur` untouched with no previous endpoint and across a fade-MODE change (a mode change is a
  discontinuity, not a ramp: blending subtractive 200 into additive 8 composites a colour the title
  never asked for). Rounds rather than truncating, or a ramp stepping by 8 keeps a half-step bias
  toward its previous level. Hermetic test, 12 checks, every branch verified by mutation.
- `Fps60` owns the endpoints and rolls them once per logic frame at the top of `present_vk`. NOT in
  `Fps60::frame_commit`: Tomba! 2's frame driver calls `presentation.commit` itself, so a capture
  there never ran at all and the endpoints stayed zero — measured, with `havePrev=0` in the log.
- Every path that composites or captures a PRESENTED frame goes through `gpu_present_fade_state`.
  The SBS readback keeps `game_render_fade_state`, because it must reproduce the real frame's
  guest-visible state.

## Evidence the fix is confined to the in-between present

Comparing the dumps before and after, over the same replay: **all 307 real frames byte-identical**,
98 of 293 in-between frames changed — the ones during a fade, and nothing else. Gameplay's numbers
are unchanged to the pixel. Oracle compare against psxport's independent Beetle full-console
reference, with widescreen and fps60 on: **34/34 checkpoints MATCH**, and the comparator's selftest
detects a seeded byte, so it can report the other answer.

## Reproduce

```sh
timeout 900 python3 tools/gate.py --debug fps60dump replay replays/bugs/walk-dust-puff.pad
mv scratch/framedump scratch/fd_product
PSXPORT_FPS60_TFORCE=0 timeout 900 python3 tools/gate.py --debug fps60dump \
    replay replays/bugs/walk-dust-puff.pad
mv scratch/framedump scratch/fd_forced
python3 external/psxport/tools/port/fps60_check.py --dir scratch/fd_product --forced scratch/fd_forced
```

## What remains

The 4.68% still not responding over that capture is 3 triples across cuts (correct) plus the
ordinary residue. Gameplay's separate 2.19% lands on animated water, foliage and the top-left gauge
— frames that flip rather than move — and is tracked in S006, not here.
