---
id: 9
title: pc_skip should skip the LOADING SCREEN entirely (not just its text)
status: todo
labels: [pc-skip, enhancement]
created: 2026-07-22
updated: 2026-07-22
---

**2026-07-22:** USER 2026-07-22: with pc_skip ON the loading screen should be skipped as a STATE - the host file read is already complete, so the screen is a wait that does not exist. Clarified after a first attempt only stopped the 'Loading.....' string from drawing (game/ui/loading_text.cpp), which leaves the same screen up for the same duration, just blank - that change is REVERTED and the behavior-map entry loading-text-skip is marked reverted. The right fork is one state up: whatever drives the loading screen's lifetime should collapse under pc_skip, in the shape 'pc_skip ? skipState() : faithfulState()'. Starting point: LoadingText::draw (FUN_8007FD54, RE'd + port_check PASS) is the drawer - find its caller and fork THAT. It ran once in the bucket capture, so ovhit on that replay will name the caller.

**2026-07-22:** GROUNDWORK (2026-07-22): the loading screen's drawer LoadingText::draw (FUN_8007FD54) has exactly ONE caller - gen_func_80044BD4, the AREA LOAD function, which is already natively owned as native_area_load_bd4 (game/core/engine.cpp:1559). IMPORTANT: that native body is already the pc_skip BYPASS - it sets sm[0x6e]/sm[0x6d]/0x1F80019B and hands off via SV_CHECK to eng(c).sop.transitionAreaLoad(), and it never runs the loading-screen body. Yet ovhit shows LoadingText::draw native=1 on the bucket capture, so the screen still appears briefly by some other route - find THAT route before forking anything. DO NOT fork native_area_load_bd4 blind: it is core loading logic on the pc_skip path, and a wrong change there breaks every area transition rather than just the cosmetic screen. Next step is to identify which path reaches FUN_8007FD54 in a pc_skip run (per-address ovhit plus a breakpoint-style probe on the caller), then fork at that path's owner.
