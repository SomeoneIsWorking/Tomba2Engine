---
id: 60
title: SEQUENCE softlock (live capture) — game keeps running, sequence never advances
status: todo
labels: [bug, pc-skip]
created: 2026-07-23
updated: 2026-07-23
evidence: docs/reference/issues/issue60_sequence_softlock.png
---

USER 2026-07-23, cut live from their running session. A SEQUENCE softlock: the game keeps advancing (frame counter climbs normally, ~30fps, no crash, no recomp-MISS) but the sequence never progresses and control is not returned. Captured state at the lock: stage=GAME(0x8010637C), sm[0x48]=2, scene-active(0x800BE258)=2 — i.e. a normal in-field scene state, not a transition. Screenshot: Tomba on a grassy slope by a tree with a bird/parrot enemy just above him (docs/reference/issues/issue60_sequence_softlock.png).

REPRO: replays/bugs/sequence-softlock-2.pad, 7049 frames from boot. Headless:
  { echo "run 7100"; echo quit; } | PSXPORT_REPL=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_PAD_REPLAY=replays/bugs/sequence-softlock-2.pad ./scratch/bin/tomba2_port
Do NOT also issue `newgame` — a pad replay starts at boot frame 0 and it desyncs.

THIS IS THE FOURTH SOFTLOCK OF THE SAME FAMILY. Read these first (`python3 tools/findings.py softlock`): #2 bucket-pickup cutscene (pc_skip ON), #5 save-sign inspect (CLOSED), #58 fisherman dialog. They share a shape — an interaction/script sequence parks forever while the frame loop keeps running.

STRONGEST PRIOR, CHECK BEFORE ANYTHING ELSE — kanban #50: under pc_skip the GAME task is a FLAT setjmp task, so a cooperative FUN_80044BD4 wait reached from inside the pc_skip GAME frame has its C++ stack DISCARDED by the longjmp; everything after the wait silently never runs. "A sequence parks forever while the loop keeps ticking" is exactly that signature, and it is why these are pc_skip-exclusive. #50's general fix (PcScheduler::spawnAndWait running the body inline on a flat task) landed 2026-07-23 — SO FIRST, RE-RUN THIS REPLAY ON CURRENT MAIN. If it no longer locks, this was #50 and the card closes. If it still locks, that is the finding: #50 deliberately did NOT convert four `native_area_load_bd4` flag==1 fire-and-forget sites, whose spawned task still runs flat — the remaining half of the same disease, and the prime suspect.

DIAGNOSIS (state, not crash — the process keeps running, so crash-checking proves nothing):
  - `PSXPORT_DEBUG=sched` — does "caught a GAME substate yield" fire at the lock frame? That is the #50 truncation signature and the single most informative probe.
  - `PSXPORT_DEBUG=seq` for the script/sequence state; DialogBoxSm (game/ui/dialog_box_sm.cpp, guest FUN_8007D594) and ScriptInterp for where it parks — #2's investigation used step counts + script-op tracing to pin the exact parked op, reuse that method.
  - REPL `watch <addr>` + PSXPORT_CW_BT=1 for a store backtrace on whatever should advance and does not.
  - The bird/parrot enemy is right next to Tomba in the capture; check whether an enemy/AI behaviour is the sequence owner (the beh_* A/B work in #10/#51 found real defects in rebuilt behaviour bodies, and 0x8013C9C0 / 0x8011D988 / 0x80121978 are still open divergences).
