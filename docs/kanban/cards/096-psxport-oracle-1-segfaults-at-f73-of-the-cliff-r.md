---
id: 96
title: PSXPORT_ORACLE=1 SEGFAULTS at f73 of the cliff replay — the reference build cannot reach the scene
status: done
labels: [bug, oracle, verification]
created: 2026-08-16
updated: 2026-08-16
---

PSXPORT_ORACLE=1 is documented as THE reference build, but on replays/bugs/cliff-fisherman-missing.pad it dies at frame 73 with signal 11, so there is no reference to compare the missing-fisherman bug against.

Backtrace, innermost first:
  GTE_WriteCR
  guest 0x80084250 / guest 0x80084250
  guest 0x8006D02C / guest 0x8006D02C     <- CutsceneCamera::lookAt's guest address
  guest 0x80078610 / guest 0x80078610
  overlay guest 0x80109450
  overlay guest 0x8010882C -> 80108784 -> 801063F4 -> 8010637C

The pc leg runs the same replay to f156 cleanly, so this is ORACLE-specific, not a scene that kills everything. Log: scratch/logs/cliff_oracle.log.

Why this matters beyond one crash: an oracle that cannot reach a scene silently removes the ability to compare there, and other results are verified against that oracle. Suspect the 2026-08-14 GTE work in psxport (a1c53d7c..25dd7826: 'Add isolated explicit-state GTE execution', 'Observe exact post-GTE results', 'Expose exact guest GTE operands to diagnostics', which rewrote much of gte_beetle.cpp) — to be established by READING, not bisecting.
