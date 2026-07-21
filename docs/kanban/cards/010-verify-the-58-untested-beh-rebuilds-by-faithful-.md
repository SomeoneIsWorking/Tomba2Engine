---
id: 10
title: Verify the ~58 untested beh_* rebuilds by faithful-body A/B (finds bugs of the confirmed class)
status: todo
labels: [verification]
created: 2026-07-22
updated: 2026-07-22
---

**2026-07-22:** Measured 2026-07-22: game/ai has 61 beh_*.cpp handlers, only 3 with an // ORACLE: marker, so ~58 have NO equivalence test against the guest. port_check cannot check them - they are rebuilds and legitimately differ in store order. The working method, demonstrated on FUN_8007DC38 (dialog driver): port_gen the byte-faithful body (PASS by construction), swap the behavior-table entry to call it, run a replay with PSXPORT_PAD_DUMP_AT, diff the 2MB dumps, swap back. Zero diff = equivalent on that path; a diff is a candidate bug of the class that produced kanban #8 and #1. Prioritise the rebuilds that actually run on replays/bugs/bucket-softlock.pad. CAUTION: PAD_DUMP_AT writes a fixed filename - rename each dump immediately or the baseline is overwritten.

**2026-07-22:** Campaign started. 0x8007DC38 beh_variant_overlay_lifecycle: 0 differing bytes. 0x800739AC beh_scene_ui_trigger (the handler that had the #5 save-sign bug): 0 differing bytes. Two down, ~56 to go, both clean - which makes a future divergence a strong signal rather than noise. Cost is two runs (~2 min) per handler. Keep the temporary faithful body in scratch/ab/ and out of the tree; wire it only for the two runs and remove it afterwards.

**2026-07-22:** 0x800741DC beh_pickup_collect_trigger (the bucket PICKUP handler itself): 0 differing bytes. Three tested, three clean. Notably this one is the handler for the very action that precedes the softlock, so it is now ruled out too.

**2026-07-22:** USE THE BUILT-IN GATE: PSXPORT_MIRROR_VERIFY=all (+ _CONTINUE=1) surveys every behaviour handler's native vs substrate legs in ONE run - the manual port_gen A/B was reinventing it. First survey over the bucket capture: exactly one mismatch, 0x80086738 Engine::installModeHandlers (v0/v1), and it is BENIGN - the only real call site overwrites v0 with 1 on the next instruction, verified in generated/shard_1.c:16529. CAVEAT: that survey run ended in SIGSEGV after continuing past the mismatch; triage whether that is the harness or a real downstream effect before trusting =all as a routine gate.

**2026-07-22:** SIGSEGV TRIAGED: not corruption and not a harness defect - it is the harness's own FATAL, 'yield inside a strict-check leg — this mirror is not yield-free; gate it with SBS lockstep instead of MV_CHECK'. A handler yielded during the replayed leg (backtrace gen_func_80044F58 -> scheduler_yield -> PcScheduler::selfClose -> VerifyHarness::strictCheck). So MIRROR_VERIFY=all cannot be a blanket survey while a yielding handler is in range; it dies at the first one. Use per-address MIRROR_VERIFY for yield-free handlers and SBS lockstep for the yielding ones. The survey still got 65 passes plus one benign mismatch before hitting it.
