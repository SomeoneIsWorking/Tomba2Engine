---
id: I020
kind: instrument
status: trusted
created: 2026-07-28
---

## Instrument

tools/render_fns.py — the STATIC counterpart to nofx: every function the game can install at node+0x18, whether or not a replay reaches it.

## Validated by

Validated by the two ways it was wrong first. (1) v1 cleared its lui/addiu map on every branch and found almost nothing; only clearing the ABI caller-saved set surfaces a fn materialised into an s-register. (2) v2 then reported 68 fns of which ~50 were fiction (page-aligned lui-only values, unaligned words, libgpu callback slots at the same offset) — the addiu-sp prologue filter removes all three classes. Survivors cross-checked against the RECOMPILER's boundaries in generated/, not Ghidra's (see I016). Final: 15 fns over 8 dumps, 4 whitelisted, 4 owned by a controller scope, 1 a real uncovered gap (0x8013D454, the water jet), 6 unconfirmed. It is a LOWER BOUND and a CANDIDATE generator — fns installed by copying from the descriptor table at 0x800A21C0 (0x80027CB4, 0x80033080, 0x8010BF54, ...) never appear, so a quiet run proves nothing.

## Known failure modes

(none recorded yet)
