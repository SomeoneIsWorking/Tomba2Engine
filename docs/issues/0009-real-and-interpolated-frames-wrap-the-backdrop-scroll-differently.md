---
id: 9
title: Real and interpolated frames apply different wrap policies to the backdrop scroll
status: open
symptom: ParallaxBg::step leaves a scroll offset one modulus out of range, while the interpolated present canonicalises it into [0, mod); on the Y axis that is not a no-op, because the Y modulus is not the drawer's period
state_items: S006
tags: tomba2,fps60,backdrop,parallax
created: 2026-09-19
updated: 2026-09-19
---

## Finding

The parallax backdrop's scroll offset passes through two different modulus policies, one per
presentation path, and until now each was documented as a copy of the other.

- **Real frame.** `ParallaxBg::step` computes a raw offset from camera yaw/pitch and reduces it with
  the guest's over-then-rollback loop (`tomba::parallax::guestReduce`). That function is the
  identity on `[0, mod)` and, outside it, returns the modulus-equivalent value one step OUT of
  range. Measured over `v` in `[-1024, 1024]` at `mod = 256`, it disagrees with a true wrap on
  **1,793 of 2,049 values**: `guestReduce(-5) = -5`, `guestReduce(600) = 344`. The result is stored
  as a signed 16-bit field at `PARALLAX_BG_SM+0x28/+0x2A` and read back with `mem_r16s`, so a
  negative or over-range offset survives to the drawer intact.
- **Interpolated frame.** `tomba_fps60_world_pass` interpolates the two captured offsets and
  canonicalises the result into `[0, mod)` (`tomba::parallax::shortestPathLerp`).

## Why the X axis is safe and the Y axis may not be

`Render::backdropRender` uses `scrollX` in exactly two places: the starting tile column,
`((scrollX - cx) >> 4) % W`, and the sub-tile pixel offset, which depends on `scrollX` only through
`(scrollX - cx) mod 16`. Its period in `scrollX` is therefore `W * 16` — which is exactly the
modulus the guest stamps at `SM+0x30` (`grid_w * 16`). **An X representation difference of one
modulus draws the identical picture.**

`scrollY` does not have that property. The Y modulus at `SM+0x32` is `(grid_h * 0x8E8) / 0x90`, not
`grid_h * 16`: for `H = 8` that is 126, while the drawer's row period is 128. Adding one Y modulus
to `scrollY` therefore shifts the backdrop by 2 px, not 0.

## What is not yet known

Whether `scrollY` ever actually leaves `[0, mod_y)` at runtime. `y = ((pitch * sY) >> 12) + H*8 -
0x20` with a signed camera pitch can clearly go negative in principle, but principle is not
evidence. If it never does, both policies are the identity and there is nothing to fix beyond the
naming, which is done.

A REPL probe was attempted and did not reach a field scene: the run stayed in the DEMO/attract
sequencer and `run 300` advanced only 90 frames. The measurement needs the replay harness
(`replays/`, the same route `tools/fps60_check.py` uses), sampling `SM+0x2A` against `SM+0x32` over
a scene with a tilemap backdrop and vertical camera motion — the seaside field.

## What has been done

`game/scene/parallax_scroll.h` now owns both policies under names that cannot be confused, with the
contract of each stated, and `tests/test_parallax_scroll.cpp` pins them apart: merging the two, or
dropping the shortest-path selection, or canonicalising the no-modulus path each fails the suite
(verified by deliberate disconnection — 2, 2 and 1 failures respectively). No behaviour changed.
