# Area index

The game has **22 areas, 0..21** — established 2026-07-22 while root-causing kanban #24, three
independent ways: 22 `A0*.BIN` files on the disc, 22 entries in every one of MAIN's six per-area
handler tables, and GAME.BIN's next-table zeroed at 22/23. Area load is
`FUN_80045080(0x80108F9C, area + 3)` — file index `area+3` into the stride-8 table at `0x800BE118`,
which is `[0]=OPN [1]=CRD [2]=SOP [3..24]=A00..A0L [25]=START [26]=DEMO [27]=GAME`.

`warp 22` and above are NOT areas. They load a non-area file into the mode slot, produce
out-of-range CD reads and then hang or wander. `warp` accepts them anyway — that missing guard is
kanban #36.

## Naming discipline (read before adding a row)

An area **index** is a fact you can measure. An area **name** is a claim, and it needs a source:
either the USER, or a name found in guest data. Two cards have already been filed against invented
names — #24 against "area 22" (does not exist) and #26 against a "Temple interior" that is a house
with framed pictures and curtained windows. If you don't have a source, leave the name blank. A
blank row is honest; a guessed one sends the next session somewhere that isn't there.

Cite the source in the row. `USER` is authoritative.

| # | Name | Source | Notes |
|---|------|--------|-------|
| 0 | | | starting seaside/field route (AUTO_SKIP lands here) |
| 1 | | | |
| 2 | | | |
| 3 | | | mine-cart level (from a sweep screenshot, not a name) |
| 4 | | | |
| 5 | | | |
| 6 | | | |
| 7 | | | |
| 8 | Water Temple | USER 2026-07-23 | |
| 9 | | | |
| 10 | | | was unreachable until the recompiler jump-table fix (kanban #27) |
| 11 | | | ditto |
| 12 | Ghost pig boss fight | USER 2026-07-23 | kanban #26 — ceiling beam band renders arced under pc_render |
| 13 | | | was unreachable until kanban #27 |
| 14 | | | ditto |
| 15 | | | |
| 16 | | | hangs under COLD warp (kanban #37); settled warp is clean |
| 17 | | | ditto |
| 18 | | | ditto |
| 19 | | | |
| 20 | | | |
| 21 | | | Tomba rides a bird; its handler was the missing `0x80109200` (kanban #24) |

## Reaching an area

Cold warp is broken (kanban #36): it self-destructs ~50 frames later — the area byte `DAT_800BF870`
resets to 0 and the wrong overlay's handler gets dispatched — and it renders black for ~45 frames
because the fade never completes. Reproduces identically under `PSXPORT_GATE=1`, so it is a
dev-tool defect, not a port defect.

The recipe that works:

```
{ echo newgame; echo "skip 3000"; echo "warp 12"; echo "skip 600";
  echo "shot scratch/screenshots/x/a.png"; echo quit; } \
| PSXPORT_REPL=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 ./scratch/bin/tomba2_port
```

Add **`PSXPORT_ORACLE=1`** for the substrate reference on the same exec leg — same execution, only the
renderer differs, which is the one clean pc-vs-psx comparison. **Not `PSXPORT_RENDER_PSX=1`**: that leg
walks the guest OT but still hands every prim to the native render queue's layer split, which needs a
pc_render producer to have published the backdrop texpage — so on that leg the guest's own 16x16
backdrop tiles land in the topmost band and paint over the whole world (kanban #78; the tile banding is
fixed now, but ORACLE is still the leg with no native ordering decision in it at all). See
`docs/driving-the-game.md` for the full REPL command set and `docs/gfx-debug.md` for the three ground
truths.

## The SETTLED camera triple per area — for resolving a USER coordinate report

The HUD's world readout is the CAMERA, not Tomba: `overlay_glue.cpp:31-33` pushes
`(int16)[0x1F8000D2], (int16)[0x1F8000D6], (int16)[0x1F8000DA]` into `rml_overlay.setWorld`. So a
coordinate in a bug report is in THAT space. `tp` writes a different quantity — Tomba's master
position `0x800E7EAC/B0/B4` (`Engine::devTeleportApply`) — so **`tp <the reported triple>` does not
go where the user was**, and `rw 800e7eac` "confirming" it only reads back the word `tp` just wrote.
See instrument I047; that circularity is what invalidated kanban #77's location sweep.

Measured 2026-08-06, `newgame / skip 3000 / warp N / skip 600 / run 300 / rw 1f8000d0 4`, no `tp`:

| area | camera (X,Y,Z) | | area | camera (X,Y,Z) |
|---|---|---|---|---|
| 0 | (3270, -1352, 2352) | | 11 | (11265, -8327, 4808) |
| 1 | (2600, -7500, 7408) | | 12 | rc=124, no capture |
| 2 | (10573, -4198, 4668) | | 13 | (7455, -1510, 7821) |
| 3 | rc=139, no capture | | 14 | (10382, -4344, 9608) |
| 4 | (7184, -8852, 4413) | | 15 | (7460, -5954, 5130) |
| 5 | (17359, -7500, 4860) | | 16 | (3433, -1704, 3502) |
| 6 | (28650, -2297, 19145) | | 17 | (3081, -5644, 4801) |
| 7 | (5456, -4102, 16670) | | 18 | (3315, -1804, 4300) |
| 8 | (5759, -4452, 8729) | | 19 | (3829, -1804, 4753) |
| 9 | (1811, -1100, 688) | | 20 | (15800, -2485, 8122) |
| 10 | (6717, -2970, 2844) | | 21 | (123, 88, 442) |

**READ THE DENOMINATOR BEFORE USING THIS.** Each row is ONE point — the camera where that area's
`warp` + settle leaves you — not the area's coordinate range. A player who walked anywhere is off
this table, so a row is evidence of a NEIGHBOURHOOD, never an identification. Areas 3 and 12 refused
to capture (rc 139 / 124) and are simply absent; they are not "no match".
Worked example: kanban #77's T1 (13029,-2872,7161) and T2 (20161,-1923,8268) match NO row (nearest
by L1 is area 20 at 4119 / 5069, and area 20 is a torchlit cave, not water and not a green mass), so
both reports are from somewhere a `warp` does not land — which is why they need a driven repro, not
a warp.
