# 075 — the native camera reads its projection constants back out of the GTE

status: open
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
