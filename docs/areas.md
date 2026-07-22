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

Add `PSXPORT_RENDER_PSX=1` for the psx_render reference on the same exec leg — same execution, only
the renderer differs, which is the one clean pc-vs-psx comparison. See `docs/driving-the-game.md`
for the full REPL command set and `docs/gfx-debug.md` for the three ground truths.
