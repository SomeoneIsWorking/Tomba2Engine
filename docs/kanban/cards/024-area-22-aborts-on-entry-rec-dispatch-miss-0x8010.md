---
id: 24
title: Area 22 aborts on entry: rec_dispatch miss 0x80109200
status: todo
labels: [bug, recomp]
created: 2026-07-22
updated: 2026-07-22
---

Found 2026-07-22 during the pc_render missing-layer area sweep (REPL `warp 22`, seaside replay). After the 0x8003BBF8 jump-table root-cause fix unblocked areas 10/11/13/14, area 22 still aborts:

  [recomp-MISS 0] no recompiled fn for 0x80109200  (caller ra=0x801087DC, a0=0x800ED058, c->pc=0x80077B38)

0x80109200 is in the GAME overlay range, not MAIN, so it is a DIFFERENT gap from the 0x80028E10 switch: the caller c->pc=0x80077B38 is a resident MAIN fn and a0=0x800ED058 is the HUD state struct. Next step: find which overlay module owns 0x80109200 and why its module's seed set misses it (overlay_data_func_pointers / code_pointer_tables), the same way external/psxport/tools/recomp/emit.py was fixed for MAIN. Evidence log: scratch/screenshots/layergap/z.log. Areas 0-21 and 23+ were not blocked by this.

**2026-07-22:** Operator spot-check 2026-07-22: warping COLD (newgame then warp 10, no intermediate play) now completes the area load - '[repl] warp: full area load for area 10 done (f28), bf870=10' - which confirms the recompiler switch-idiom fix really did unblock the area itself; the old failure was earlier, at 0x80028FA4/FC4 from FUN_80028E10. But the cold warp then aborts on a DIFFERENT miss, 0x80109450 (caller ra=0x801088B0), which is the SOP-overlay signature address the render path also tests. That is adjacent to this card's own 0x80109200, so both are likely the same class: an OVERLAY-side gap that a cold warp exposes because the overlay is not resident, where the sweeping agent's runs reached the areas by a path that had it loaded (it captured y10..y14 renders). Worth confirming which it is before treating them as separate bugs - if cold-warp is simply an unsupported entry, say so on the card rather than chasing a phantom; if the overlay should be demand-loaded, that is the real fix and it covers both addresses.
