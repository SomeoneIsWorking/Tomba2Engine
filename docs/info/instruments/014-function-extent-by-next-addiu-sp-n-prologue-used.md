---
id: I014
kind: instrument
status: DISTRUSTED
created: 2026-07-28
distrusted_on: 2026-07-28
---

## Instrument

function-extent by 'next addiu sp,-N prologue' (used by the ad-hoc static census scans)

## Validated by

UNSOUND but audited clean for the decisions it drove. It over-runs past a function's 'jr ra' into whatever follows, because a function that ends without another prologue immediately after keeps scanning. Caught 2026-07-28 when Ghidra decompiled 0x8002ECD8 as ending in a tail-call to Trig::rsin (0x80083E80) — impossible for a sprite emitter; the raw instructions show the function actually runs to 0x8002EF58 and its last call is JAL 0x8002E680. The sound detector is 'first jr ra + its delay slot'. RE-AUDITED every function the kanban #15 CR0-7 census judged: 2 of 11 were mis-sized (0x80028B70 heuristic 1564 vs real 672; 0x80030D68 916 vs 908) but BOTH still write all of CR0-7 inside the correct shorter extent, so their SCOPE-READY verdict stands; and 0x8002F36C — the one controller EXCLUDED from the batch — measures 424 bytes under both methods with genuinely zero CR writes, so that exclusion was correct too. No shipped decision changed. Use the jr-ra detector for any future census.

## Known failure modes

(none recorded yet)

## DISTRUSTED 2026-07-28

Over-runs past jr ra into the following function. Audited clean for the #15 census decisions, but use the 'first jr ra + delay slot' detector instead.

> Every result this instrument produced is suspect until it is re-validated.
