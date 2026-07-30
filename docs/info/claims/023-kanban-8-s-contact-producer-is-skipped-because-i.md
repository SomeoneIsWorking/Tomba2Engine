---
id: C023
kind: claim
status: holds
created: 2026-07-30
tags: ai
---

## Claim

kanban #8's contact producer is skipped because its aux-list count at scratchpad 0x1F800144 is zero for the whole replay, not at one frame phase

## Evidence

MEASURED 2026-07-30 over replays/bugs/seesaw-weight.pad: the byte at scratchpad 0x1F800144 — the count that ov_a00_gen_801130C4 reads (mem_r8(0x1F800000+324)) and branches on — is 0x00 at ALL 14 samples taken every 500 frames from frame 500 to 6500, which spans the grab state at ~6424. So the producer loop is skipped for the ENTIRE replay, not at one unlucky frame phase. This resolves card #8's open note 'Aux list count at 0x1F800144 reads 0 in the .spad at the dump instant (unresolved whether that is frame-phase)': it is not frame-phase. OBSERVATION, NOT A CONCLUSION: the list POINTER at 0x1F80013C is a live-looking 0x800F2410, and the neighbouring byte at 0x1F800146 DOES vary across the run (0x07 at f500, 0x0C at f1000, 0x06 from f1500 on). So something list-shaped is being maintained nearby while the byte the producer reads stays zero. Whether 0x146 is the real count and 0x144 is the wrong field, or 0x144 is correct and simply never populated, is NOT established and needs the writer of those bytes identified.

## What would falsify it

a sample of 0x1F800144 on a different replay or scene showing a nonzero value, or identification of a writer that sets it nonzero under conditions this replay never reaches
