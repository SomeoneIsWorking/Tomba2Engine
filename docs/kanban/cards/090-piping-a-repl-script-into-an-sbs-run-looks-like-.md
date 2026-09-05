---
id: 90
title: Piping a REPL script into an SBS run LOOKS like it works and drives NOTHING — silently discarded
status: done
labels: [bug, verification, instrument]
created: 2026-08-12
updated: 2026-08-12
---

2026-08-12. Under PSXPORT_SBS_MODE=full the REPL is never pumped: Game::repl.read() has exactly ONE caller, external/psxport/runtime/psx/native_boot.cpp (the SINGLE-CORE frame loop), and sbs.cpp never reads stdin (its 7 'repl' greps are all incidental — pad repl_on/repl_hold state, the words 'replay' and 'replacing'). So 'printf newgame/run 200/quit | PSXPORT_SBS_MODE=full ./scratch/bin/tomba2_port' silently discards every command and runs a plain no-autonav lockstep from boot. Verified by reading both files, and corroborated by scratch/logs/sbs.log (2026-08-04) which states 'LOCKSTEP from boot (no auto-nav)' for the same invocation shape.

WHY THIS IS AN INSTRUMENT DEFECT AND NOT A MISSING FEATURE: I used exactly this invocation earlier today believing I had driven a newgame under SBS, and reported its crash as an SBS+newgame bug (card #86). The run had never left attract mode, so the whole premise of that card was an artifact of input that was accepted and thrown away. Silently-skipped input is a failure, not a filter.

FIX, either direction is acceptable but doing NOTHING is not: (a) pump the REPL from the SBS loop too, or (b) REFUSE at startup — if stdin is not a TTY and SBS mode is on, print that the REPL is not serviced under SBS and name PSXPORT_SBS_AUTONAV as the way to drive it, and exit non-zero. (b) is cheap and kills the false-confidence path immediately. Use AUTONAV until then.

**2026-08-12:** **CLOSED 2026-08-12 — fix (b) landed and is VERIFIED IN THIS TREE, both directions.** psxport daf4e026 added runtime/psx/repl_service.{h,cpp} (refuse_if_unserviced); 6edea2c0 extended the guard to the DualCore and selftest loops, which the first commit left unguarded. Pinned here at 0a6c90f9.

A discriminator is only trusted after being run against BOTH classes, so both were run on scratch/bin/tomba2_port built from that pin:

POSITIVE (the bug's own invocation) — printf 'newgame\nrun 200\nquit\n' | PSXPORT_SBS_MODE=full ./scratch/bin/tomba2_port -> **rc=2**, refusing before producing any verdict. It names the denominator (21 pending stdin bytes), names every working alternative (PSXPORT_SBS_AUTONAV=1, PSXPORT_SBS_WARP, PSXPORT_SBS_PAD_REPLAY, PSXPORT_DEBUG_SERVER=1, or the single-core port for the REPL itself), and states its own blind spot out loud: 'this cannot see a driver that has written nothing yet; the check re-runs every frame'. Log: scratch/logs/repl_refusal.log.

NEGATIVE (a legitimate run must NOT be refused) — PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 ./scratch/bin/tomba2_port < /dev/null -> **0 occurrences of repl:error**, ran real lockstep work to the 120s harness timeout (rc=143 = SIGTERM from the wall clock, NOT a refusal) and emitted its normal coverage line '251/482 owned addresses executed'. Log: scratch/logs/sbs_nopipe.log.

So the false-confidence path that made card #86's premise an artifact is closed by construction: that invocation can no longer produce a verdict at all.
