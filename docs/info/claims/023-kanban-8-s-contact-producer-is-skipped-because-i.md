---
id: C023
kind: claim
status: falsified
created: 2026-07-30
tags: ai
falsified_on: 2026-07-30
---

## Claim

kanban #8's contact producer is skipped because its aux-list count at scratchpad 0x1F800144 is zero for the whole replay, not at one frame phase

## Evidence

MEASURED 2026-07-30 over replays/bugs/seesaw-weight.pad: the byte at scratchpad 0x1F800144 — the count that ov_a00_gen_801130C4 reads (mem_r8(0x1F800000+324)) and branches on — is 0x00 at ALL 14 samples taken every 500 frames from frame 500 to 6500, which spans the grab state at ~6424. So the producer loop is skipped for the ENTIRE replay, not at one unlucky frame phase. This resolves card #8's open note 'Aux list count at 0x1F800144 reads 0 in the .spad at the dump instant (unresolved whether that is frame-phase)': it is not frame-phase. OBSERVATION, NOT A CONCLUSION: the list POINTER at 0x1F80013C is a live-looking 0x800F2410, and the neighbouring byte at 0x1F800146 DOES vary across the run (0x07 at f500, 0x0C at f1000, 0x06 from f1500 on). So something list-shaped is being maintained nearby while the byte the producer reads stays zero. Whether 0x146 is the real count and 0x144 is the wrong field, or 0x144 is correct and simply never populated, is NOT established and needs the writer of those bytes identified.

## What would falsify it

a sample of 0x1F800144 on a different replay or scene showing a nonzero value, or identification of a writer that sets it nonzero under conditions this replay never reaches

## FALSIFIED 2026-07-30

MEASURED WRONG BY ITS OWN NEXT STEP. Wiring 0x80111304 and reading ovhit shows native=1151 oracle=1151 over 1500 frames of the same replay — the producer RUNS, roughly once per frame. C023 concluded it was 'skipped for the entire replay' because the aux-list count at scratchpad 0x1F800144 is zero, which was correctly measured but wrongly generalised: 0x801130C4 is only ONE OF EIGHT distinct callers of 0x80111304 (the others being 0x801131A4, 0x801131B8, 0x801131CC, 0x801132C8, 0x801132D8, 0x801332EC, 0x80113314 — mid-loop entry clones). The zero count gates that one path, not the function. The real question therefore moves BACK to the producer's own gate: it writes item+0x2B=1 only when the overlap test 0x8002300C returns nonzero, so with 1151 calls and no nonzero contact index ever observed, either that overlap test fails every time for these objects or a clearer zeroes the stamp within the same frame. Neither is measured yet.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
