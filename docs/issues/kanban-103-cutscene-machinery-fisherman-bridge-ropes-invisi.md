---
id: kanban-103
title: Cutscene machinery + fisherman + bridge ropes invisible under pc_render (area 0, live)
status: open
labels: [render, bug]
---


USER 2026-08-19, live windowed run (pc_faithful + pc_render), area 0, pad frame ~30150 of replays/bugs/machinery-invisible.pad (cut from the live session with padrec save). A cutscene where an NPC works a piece of MACHINERY: the machinery is INVISIBLE, the FISHERMAN the user expects in the scene is absent, and the user also reports the BRIDGE ROPES missing.

Evidence: scratch/screenshots/live/mach_now.png (live shot) and scratch/screenshots/mach/pc_30150.png (the replay reproducing it bit-for-bit on the default leg — determinism confirmed, so this is a headless repro).

Repro: PSXPORT_PAD_RESUME=replays/bugs/machinery-invisible.pad ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE  (fast-forwards ~4 min to the scene, then hands over; add PSXPORT_DEBUG_SERVER=<port> to drive it).

NOT comparable against PSXPORT_ORACLE=1 by replaying the same pad: measured 2026-08-19, the oracle leg needs 56370 native frames to reach pad frame 30150 where the default leg needs 31050, so the recorded presses land at different moments and the oracle leg ended up on a save prompt instead of the cutscene (scratch/screenshots/mach/oracle_30150.png). Use the live renderpath switch on ONE running game instead.

SUSPECTED FAMILY, not yet confirmed: the rope/tether producers. #56 (no line-primitive producer) covered ropes/fishing line; #95 (cliff fisherman body+rod absent) root-caused to GuestQueueDispatch::guestFlushesMesh answering from the jump-table ARM alone; #97 (tether producer dispatched by TYPE byte with no queue/head gate) is still open. A missing NPC + missing machinery + missing bridge ropes in one scene is the shape of ONE shared producer gap, like #56 was.
