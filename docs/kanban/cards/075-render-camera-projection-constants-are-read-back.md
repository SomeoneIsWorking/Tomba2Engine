# 075 — the native camera reads its projection constants back out of the GTE

status: done
created: 2026-08-05
tags: render, camera, gte, user-rule, debt
where: `game/render/scene_build.cpp` `NativeScenePass::collect()`

## What is wrong

```cpp
out->cam.ofx = (float)(int32_t)gte_read_ctrl(24) / 65536.0f;
out->cam.ofy = (float)(int32_t)gte_read_ctrl(25) / 65536.0f;
out->cam.H   = (float)(uint16_t)gte_read_ctrl(26);
```

Three reads of PSX GTE control registers — engine state, after the substrate ran. This is the
pattern the USER banned outright ("never do this please NEVER, just leaving the effect as is is
better than this"). It is milder than a recovered object transform (these are constants the game
SET, not a transform reconstructed post-hoc), which is exactly why it survived the tap sweep — but
it is the same shape, it is inherited by **every** producer including the `cube_text_banner.cpp`
exemplar that is held up as tap-free, and it silently couples the native camera to whatever the
guest last left in the GTE.

## Why this one is cheap — the values are already ours at the SOURCE

`docs/engine-ownership-audit.md` rows 40-41:

| addr | routine | native override | status |
|---|---|---|---|
| `0x800846D0` | SetGeomOffset (OFX/OFY) | `ov_set_geom_offset` | 0-diff (OFX 160 / OFY 120) |
| `0x800846F0` | SetGeomScreen (H/FOV) | `ov_set_geom_screen` | 0-diff (H = 350) |

Both leaves are RE'd and were byte-verified. The game sets these from its projection setup at
`0x800509B4` (`eng_init_display`), which calls both. So the port can capture the arguments where the
game STATES them and never ask the GTE what it currently holds.

**Note before starting:** `grep -rn 'ov_set_geom_offset|ov_set_geom_screen' game/` returns NOTHING
today — those overrides are named in the audit but are not currently registered in this repo, so the
setters run on the substrate. Standing them back up is part of this task, not a precondition someone
else already met. `game/world/pool.cpp:289` still calls `0x800846F0` directly for the area draw
range, which is a second writer and must feed the same capture.

## The fix

1. A small game-owned `CameraParams { float ofx, ofy; float H; bool set; }` on the game context.
2. Register overrides on `0x800846D0` and `0x800846F0` that read the arguments (a0/a1 = ofx/ofy;
   a0 = h), record them, then run the substrate body so the GTE stays correct for anything still
   reading it. Widescreen already widens OFX inside the offset setter — keep that in ONE place and
   have the capture take the widened value, or the 3D reprojects one setter-call late (journal 5900).
3. `collect()` reads `CameraParams`, never `gte_read_ctrl`.
4. If `!set` when a frame is collected, that is an RE gap — say so loudly and refuse, per this
   repo's "no fabricated behaviour" rule. Do NOT fall back to the GTE read; a fallback would make
   this look fixed while the ban is still being violated on the path that matters.

## How to know it worked

- `grep -rn 'gte_read_ctrl' game/render/` returns zero hits in the camera path.
- Same rendering before/after at a fixed viewpoint: OFX 160 / OFY 120 / H 350 are the audited
  values, so a correct capture reproduces the current image exactly. A DIFFERENT image means the
  capture is wrong, not that the fix is working.
- Widescreen still anchors correctly (that is what OFX widening exists for) — check a pause menu /
  2D-over-3D screen in wide, per journal 5926.

## Why it is filed rather than done

Found and scoped 2026-08-05 while closing the audio and A/V-sync work in the sibling ports. Not
started: doing it properly means standing up two overrides in this repo and verifying against a real
run, and it was not safe to begin an edit here that could not be built and verified in the same
session. Everything needed to execute it is above — no re-derivation required.

## DONE 2026-08-05 — and three things this card had wrong

Landed as psxport `5f3d7bf7` + the game-side commit that bumps to it. The fix is what section "The
fix" describes, at a different ALTITUDE and in a different REPO. What the card got wrong, recorded
because each one would have sent the next session down a narrower path than the real one:

1. **`scene_build.cpp` was not the reader that mattered.** The camera path's real choke is
   `Fps60::sceneCam` — in **psxport**, not this repo — and EVERY native producer inherits it
   (projection.cpp, native_terrain.cpp, cube_text_banner.cpp, submit.cpp). `scene_build.cpp` was a
   second, duplicating reader. Fixing only the three lines this card names would have left the
   framework-level GTE read serving the whole rest of the render path: a cosmetic fix.

2. **`ov_set_geom_offset` / `ov_set_geom_screen` were not "overrides that need standing back up".**
   The setters had been ported INTO `Engine::initDisplay` (`game/scene/startup.cpp`), which writes
   CR24/25/26 inline. The audit rows describe a shape that no longer exists. Overrides were still
   needed, but for a different reason than the card gives — see 3.

3. **There were THREE writers of the projection, not two.** Besides `Engine::initDisplay` and
   `Pool::finalViewInit`, the substrate calls the setters directly (`func_800846D0` in shard_3/
   shard_7, `func_800846F0` in shard_0/shard_3/shard_5), AND `native_step_frame` re-asserts CR24
   every frame under widescreen because the window is created lazily. Capturing only at the two
   named callers would have left the GTE moving without the port's copy — the exact desync the
   change removes. All three now go through the one implementation.

**It is framework code, not game code.** Both leaves are pure libgte (`CR24 = ofx<<16; CR25 =
ofy<<16` and `CR26 = h`), identical in every game that links libgte, and their consumer (ProjParams,
Fps60::sceneCam) was already framework. So the behaviour lives in psxport and only the ADDRESS is
per-game (`GameConfig::hle.setGeomOffset/.setGeomScreen`), registered by PlatformHle — which is
correct here precisely because these touch NO guest RAM, so firing on the SBS oracle leg too is fine.
A first attempt put a `LibgteGeom` class in `game/render/`, following the convention of
`libgpu_draw_env.cpp` / `mtx.cpp`; the USER challenged it and the convention did not survive
scrutiny.

**Acceptance gate, met as stated:** three VK-headless shots at fixed REPL frames are BIT-IDENTICAL to
the pre-change baseline (`cmp`). That is the right answer, not a null result — 160/120/350 are the
audited values, so a correct capture reproduces the image exactly and a DIFFERENT image would have
meant the capture was wrong. `grep -rn 'gte_read_ctrl(2[456])' game/` is now zero live hits (one
comment in `submit.cpp` explaining why not to do it). Framework ctest 14/14.

**What makes "identical" evidence rather than coincidence:** `requireGeom` ABORTS on an unset
projection instead of falling back. Had the capture not happened, the run would have died with a
backtrace rather than rendering. Measured alongside: `PSXPORT_DEBUG=ovhit` showed the screen setter
reached 3x through dispatch before the move to PlatformHle, so the substrate-caller path is live and
not theoretical.

**Deliberately NOT done:** `proj_native_xform` / `proj_native_vertex` (gte_beetle.cpp) still read
CR24/25/26, and that is correct — they are native reimplementations of the GTE's own RTPS/RTPT
instruction, so the control registers ARE their input. The ban is on the CAMERA recovering a
projection it was handed, not on the GTE emulation reading the GTE.
