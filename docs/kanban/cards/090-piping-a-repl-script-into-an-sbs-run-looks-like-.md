---
id: 90
title: Piping a REPL script into an SBS run LOOKS like it works and drives NOTHING — silently discarded
status: todo
labels: [bug,verification,instrument]
created: 2026-08-12
updated: 2026-08-12
---

2026-08-12. Under PSXPORT_SBS_MODE=full the REPL is never pumped: Game::repl.read() has exactly ONE caller, external/psxport/runtime/recomp/native_boot.cpp (the SINGLE-CORE frame loop), and sbs.cpp never reads stdin (its 7 'repl' greps are all incidental — pad repl_on/repl_hold state, the words 'replay' and 'replacing'). So 'printf newgame/run 200/quit | PSXPORT_SBS_MODE=full ./scratch/bin/tomba2_port' silently discards every command and runs a plain no-autonav lockstep from boot. Verified by reading both files, and corroborated by scratch/logs/sbs.log (2026-08-04) which states 'LOCKSTEP from boot (no auto-nav)' for the same invocation shape.

WHY THIS IS AN INSTRUMENT DEFECT AND NOT A MISSING FEATURE: I used exactly this invocation earlier today believing I had driven a newgame under SBS, and reported its crash as an SBS+newgame bug (card #86). The run had never left attract mode, so the whole premise of that card was an artifact of input that was accepted and thrown away. Silently-skipped input is a failure, not a filter.

FIX, either direction is acceptable but doing NOTHING is not: (a) pump the REPL from the SBS loop too, or (b) REFUSE at startup — if stdin is not a TTY and SBS mode is on, print that the REPL is not serviced under SBS and name PSXPORT_SBS_AUTONAV as the way to drive it, and exit non-zero. (b) is cheap and kills the false-confidence path immediately. Use AUTONAV until then.
