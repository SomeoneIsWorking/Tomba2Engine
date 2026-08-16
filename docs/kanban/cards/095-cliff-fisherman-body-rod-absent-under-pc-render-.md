---
id: 95
title: Cliff fisherman: body + rod absent under pc_render while his fishing LINE still draws
status: done
labels: [bug, render]
created: 2026-08-16
updated: 2026-08-16
---

USER 2026-08-16, live windowed run, paused at the spot. A seaside CLIFF plateau over water: the fisherman NPC and the ROD he holds are both missing; only the fishing LINE renders, and it is nearly invisible because it blends with the water. USER: "there is no rod either, actually the fishing line is there but you can't see it because it blends with the water".

The asymmetry IS the clue: the line effect (0x5E line/rope family, game/render/fx_line.cpp) produces geometry every frame, so the entity is live — this is not a failed spawn. Something drops the MESH between submission and the picture.

REPRO (deterministic, headless, 156 frames, cut from the user's live session with padrec):
  printf 'run 156\nshot scratch/screenshots/x.png\nquit\n' | PSXPORT_REPL=1 PSXPORT_NO_FMV=1 \
    PSXPORT_NOPACE=1 PSXPORT_NOAUDIO=1 PSXPORT_PAD_REPLAY=replays/bugs/cliff-fisherman-missing.pad \
    PSXPORT_SETTINGS=psxport_settings.ini ./scratch/bin/tomba2_port
Evidence: scratch/screenshots/live/spot.png (the user's live shot) and cliff_pc.png (the replay).

NOT the camera regression fixed in 3d4bfb7 — that made the ENTIRE world invisible and is fixed; the world renders here.

Oracle compare is BLOCKED in this scene: PSXPORT_ORACLE=1 segfaults at f73 (see the sibling card).
