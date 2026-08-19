---
id: 103
title: Cutscene machinery + fisherman + bridge ropes invisible under pc_render (area 0, live)
status: doing
labels: [render, bug]
created: 2026-08-19
updated: 2026-08-19
---

USER 2026-08-19, live windowed run (pc_faithful + pc_render), area 0, pad frame ~30150 of replays/bugs/machinery-invisible.pad (cut from the live session with padrec save). A cutscene where an NPC works a piece of MACHINERY: the machinery is INVISIBLE, the FISHERMAN the user expects in the scene is absent, and the user also reports the BRIDGE ROPES missing.

Evidence: scratch/screenshots/live/mach_now.png (live shot) and scratch/screenshots/mach/pc_30150.png (the replay reproducing it bit-for-bit on the default leg — determinism confirmed, so this is a headless repro).

Repro: PSXPORT_PAD_RESUME=replays/bugs/machinery-invisible.pad ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE  (fast-forwards ~4 min to the scene, then hands over; add PSXPORT_DEBUG_SERVER=<port> to drive it).

NOT comparable against PSXPORT_ORACLE=1 by replaying the same pad: measured 2026-08-19, the oracle leg needs 56370 native frames to reach pad frame 30150 where the default leg needs 31050, so the recorded presses land at different moments and the oracle leg ended up on a save prompt instead of the cutscene (scratch/screenshots/mach/oracle_30150.png). Use the live renderpath switch on ONE running game instead.

SUSPECTED FAMILY, not yet confirmed: the rope/tether producers. #56 (no line-primitive producer) covered ropes/fishing line; #95 (cliff fisherman body+rod absent) root-caused to GuestQueueDispatch::guestFlushesMesh answering from the jump-table ARM alone; #97 (tether producer dispatched by TYPE byte with no queue/head gate) is still open. A missing NPC + missing machinery + missing bridge ropes in one scene is the shape of ONE shared producer gap, like #56 was.

**2026-08-19:** 2026-08-19: the machinery is NOT drawn through the generic GT3/GT4 emitter. Widening the per-object redirect to func_800803DC (perobj_dispatch.cpp, the shape framework doc generic-object-producer.md describes) was verified non-regressive — A/B at a FIXED frame, AUTO_SKIP free-roam f400, 0 of 76800 px differ, with the redirect channel confirming it fired on the widened leg and not on the base leg — but the cutscene still renders without the machine (scratch/screenshots/mach/gen_cut_crop.png). So its submitter is one of the other 26 unowned rows.

NEXT STEP, and it is blocked on a tool gap: the packet->submitter lookup is 'otattr', which is REPL-ONLY (repl.cpp), and the REPL blocks the frame loop so it cannot attach to a live or long-resumed session. Put otattr on the DEBUG SERVER (same gap renderpath had, same fix), then: renderpath psx -> provat a machinery pixel -> read the packet address -> otattr that address -> the span's fn IS the submitter. Then own that submitter.

Do NOT re-derive the scene: PSXPORT_PAD_RESUME=replays/bugs/machinery-cutscene.pad reaches it in ~5 min.
