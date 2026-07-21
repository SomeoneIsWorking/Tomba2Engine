---
id: 6
title: Load-Game browser (DEMO s48==4) aborts: rec_dispatch miss 0x8018FA88
status: done
labels: [bug, recomp]
created: 2026-07-21
updated: 2026-07-21
---

Selecting into the title Load-Game browser aborts. Path: ov_demo_gen_801062E4 -> FUN_8007BF20 (the s48==4 handler) -> FUN_8007BE18 -> rec_dispatch(0x8018FA88) -> [recomp-MISS 0] no recompiled fn. Diagnosis: 0x8018FA88 is in NO modeled overlay slot ('addr not in any slot range'). The 0x8018xxxx stage slot holds OPN.BIN (13596 B @0x8018A000 -> ends 0x8018D51C); the target is ~23KB in, so a larger/other overlay must load there that we never extract+recompile (all 28 extracted overlays are accounted for; area overlays live at 0x8010xxxx). The call is a LITERAL target in MAIN (shard_4.c:11667), so the game really calls it. REPRO: boot headless, no replay, tap right then x at the title, run ~60 frames. BLOCKS: verification of Render::renderCardBrowser (s48==4 producer) — the substate crashes before rendering. FIX: identify the overlay resident at 0x8018A000 in that context and add it to the recompiled set (tools/ensure_recomp.py ALL_OVERLAYS / emit.py EXTRA_SEEDS), or widen the slot range if OPN has a larger variant.
