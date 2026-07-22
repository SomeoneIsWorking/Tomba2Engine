---
id: 24
title: Area 22 aborts on entry: rec_dispatch miss 0x80109200
status: done
labels: [bug, recomp]
created: 2026-07-22
updated: 2026-07-22
---

Found 2026-07-22 during the pc_render missing-layer area sweep (REPL `warp 22`, seaside replay). After the 0x8003BBF8 jump-table root-cause fix unblocked areas 10/11/13/14, area 22 still aborts:

  [recomp-MISS 0] no recompiled fn for 0x80109200  (caller ra=0x801087DC, a0=0x800ED058, c->pc=0x80077B38)

0x80109200 is in the GAME overlay range, not MAIN, so it is a DIFFERENT gap from the 0x80028E10 switch: the caller c->pc=0x80077B38 is a resident MAIN fn and a0=0x800ED058 is the HUD state struct. Next step: find which overlay module owns 0x80109200 and why its module's seed set misses it (overlay_data_func_pointers / code_pointer_tables), the same way external/psxport/tools/recomp/emit.py was fixed for MAIN. Evidence log: scratch/screenshots/layergap/z.log. Areas 0-21 and 23+ were not blocked by this.

**2026-07-22:** Operator spot-check 2026-07-22: warping COLD (newgame then warp 10, no intermediate play) now completes the area load - '[repl] warp: full area load for area 10 done (f28), bf870=10' - which confirms the recompiler switch-idiom fix really did unblock the area itself; the old failure was earlier, at 0x80028FA4/FC4 from FUN_80028E10. But the cold warp then aborts on a DIFFERENT miss, 0x80109450 (caller ra=0x801088B0), which is the SOP-overlay signature address the render path also tests. That is adjacent to this card's own 0x80109200, so both are likely the same class: an OVERLAY-side gap that a cold warp exposes because the overlay is not resident, where the sweeping agent's runs reached the areas by a path that had it loaded (it captured y10..y14 renders). Worth confirming which it is before treating them as separate bugs - if cold-warp is simply an unsupported entry, say so on the card rather than chasing a phantom; if the overlay should be demand-loaded, that is the real fix and it covers both addresses.

**2026-07-22:** ROOT-CAUSED AND FIXED (recompiler), plus the card's premise corrected on two counts.

(1) The miss 0x80109200 belongs to AREA 21 (hut interior, ov_a0l), not area 22 — reproduced exactly: 'warp 21' -> recomp-MISS 0x80109200 (caller ra=0x801087DC, a0=0x800ED058), the same signature this card recorded.

(2) CAUSE, and it is the same CLASS as #27 but a different mechanism. The game keeps SIX parallel per-area dispatch tables in RESIDENT MAIN's data, indexed by the area byte DAT_800BF870: 0x8009D1D4, 0x8009D22C, 0x800A45B8, 0x800A4AA0, 0x800A4AF8, 0x800A4B50, 22 entries each, where entry i is an address inside overlay i's image. Neither per-module scan can see them — MAIN's pointer scans reject the words (outside MAIN's text) and each overlay's own scan never reads MAIN's data. Every entry was invisible to discovery. The project had been patching this ONE ADDRESS AT A TIME: emit.py's hand-written OVERLAY_EXTRA_SEEDS A00..A0L block was literally table 0x800A45B8 transcribed by hand over three sessions.

FIX (external/psxport/tools/recomp/emit.py, RECOMP_VERSION 2026-07-22.2): area_indexed_overlay_tables() discovers the tables from the binary — a window of N consecutive MAIN words (N = number of area overlays) is an area table when word i is inside overlay i's image and >=80% are valid function entries in their OWN overlay. Per-index bounds is the discriminator (overlay sizes differ >10x). Only real entries are seeded, so a stale slot can't split a function. Acceptance includes the NULL HANDLER (a bare 'jr ra' at a call-table target) — 0x80109200 is exactly that, and it matches neither of is_func_entry's signals. The hand-written jt block was deleted.

SURGICAL, proven: the emitted set was diffed module-by-module against the pre-change generation. Exactly two deltas across all 28 modules — +ov_a0l gen_80109200 (the bug) and -ov_a03 gen_801127EC (index 3 is a stale duplicate of A04's address in all six tables; in A03 it points at the data word 0x000006FF, so the old hand seed was emitting a function out of data). Everything else identical.

VERIFIED: warp 21 from a settled run renders (scratch/screenshots/sweep/z21.png), 0 misses. Areas 0-21 all load, 0 misses. Area 3 unaffected (z3.png). Boot smoke replays/boot-smoke/short-session.pad 3000 frames clean on BOTH pc_faithful+pc_render and PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1.

(3) AREA 22 IS NOT A FIELD AREA. The game has 22 areas, 0..21. Area load is file index area+3 into the table at 0x800BE118, dumped live as [0]=OPN [1]=CRD [2]=SOP [3..24]=A00..A0L [25]=START [26]=DEMO [27]=GAME. So warp 22 loads START.BIN into the mode slot (hence the 'LBA out of range' reads and the garbage stage). Corroborated by there being exactly 22 A0*.BIN overlays, all six MAIN tables having 22 entries, and GAME.BIN's nexttab having [22]=[23]=0. Swept 22,23,24,28,29,30,31: all load a non-area file and hang/wander, NONE gives a rec_dispatch miss. warp should reject id>=22 — see the new card.

Full write-up: docs/findings/scene.md.
