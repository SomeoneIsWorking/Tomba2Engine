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
