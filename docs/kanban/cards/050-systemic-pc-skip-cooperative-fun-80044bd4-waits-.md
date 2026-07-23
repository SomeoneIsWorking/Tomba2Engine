---
id: 50
title: SYSTEMIC pc_skip: cooperative FUN_80044BD4 waits inside the GAME frame are SILENTLY TRUNCATED
status: todo
labels: [bug, pc-skip]
created: 2026-07-23
updated: 2026-07-23
---

Found 2026-07-23 while root-causing #47. THIS IS A CLASS DEFECT, not one bug, and it is the mechanism behind the user's 'pc_skip exclusive' observation.

MECHANISM (RE'd, proven three independent ways): under pc_skip the GAME task is a FLAT setjmp task. So when code reached from inside the pc_skip GAME frame parks on a cooperative wait — FUN_80044BD4 / the spawnAndWait shape — scheduler_yield LONGJMPS back into PcScheduler::runGameStanza and DISCARDS THE C++ STACK. Everything after the wait in that C++ frame never runs. Under pc_faithful the GAME runs on the stage FIBER, where the yield resumes normally and the tail executes — which is exactly why these bugs are pc_skip-exclusive and why a faithful-leg compare shows nothing.
Proof used on #47: PSXPORT_DEBUG=sched prints 'caught a GAME substate yield' once at f661; the hook's post-spawn store 0x1F80019C=0 never fires; a byte watch on the target state shows no write. Three independent confirmations of the truncation.

BLAST RADIUS: #47's area-0 transition-ENTER hook was one instance. THE ENTER HOOKS FOR AREAS 1..21 CARRY THE SAME TRUNCATION — so an unknown number of per-area transition end-states are silently not being established under the default config. Six open-coded 'native_area_load_bd4' collapses already exist in the tree as one-off workarounds for this same class (and #47's fix just added a seventh) — i.e. the codebase has been treating instances instead of the cause.

THE GENERAL FIX (the point of this card): make PcScheduler::spawnAndWait run the spawned body INLINE on a flat task, in external/psxport/runtime/recomp/pc_scheduler.cpp, so a cooperative wait reached from the pc_skip GAME frame completes instead of truncating. That would (a) fix every un-found instance across areas 1..21 at once, and (b) let all SEVEN open-coded collapses be DELETED per no-tombstones — the debt exists only because the cause was untreated.
RISK: this is the scheduler, it affects every path, and it must not disturb pc_faithful (which works today) or SBS byte-exactness. Gate it hard: SBS-full 0-diff, boot smoke, the area sweep (areas 0..21 load), the dialog/menu gates, and specifically re-verify #47's hook still fires its tail with the open-coded collapse REMOVED (that is the proof the general fix subsumes the special case).
SEQUENCING: do this BEFORE hunting more individual truncation instances — each one found and hand-collapsed is wasted work the general fix would have covered.
