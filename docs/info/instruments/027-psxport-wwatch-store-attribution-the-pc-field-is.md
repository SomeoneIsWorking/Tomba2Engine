---
id: I027
kind: instrument
status: trusted
created: 2026-07-30
---

## Instrument

PSXPORT_WWATCH store attribution - the pc field is STALE and misattributes writers

## Validated by

DISTRUSTED FOR pc, TRUSTWORTHY FOR ra. mem.cpp:86 logs pc from c->pc and ra from c->r[31]. c->pc is written ONLY by the generated thunks, so it goes STALE the moment a callee returns - every store the CALLER executes after that return is logged against the last callee's address. Demonstrated on an existing log: scratch/logs/ww_tilt.log has FOUR different pc values sharing one ra=8012FCD4 (pc=80130788 x392, pc=80083EBC x40, pc=80075E04 x2, pc=8012F5B4 x1). One writer, four stale pcs. The 392 stores attributed to 0x80130788 are actually substate1Tick's own jal-delay-slot store at guest 0x8012FCD0, proven because 0x80130788 never assigns r[31] (0 call sites) so a store it executed could only log ra as one of its three callers' constants, and none appear. Corroborating tell: every logged value was under 0x1000, exactly what the caller's 4095-mask produces and not what a plus/minus 0x100 or divide-derived acceleration looks like. RULE: attribute a wwatch store by its ra, never by its pc; if the ra is not a call-site constant of the function you are blaming, you are blaming the wrong function.

## Known failure modes

(none recorded yet)
