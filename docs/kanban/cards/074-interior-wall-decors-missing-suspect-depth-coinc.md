---
id: 74
title: Interior wall decors missing (suspect depth / coincident-face ordering)
status: todo
labels: [render]
created: 2026-08-04
updated: 2026-08-04
---

**2026-08-04:** USER-REPORTED 2026-08-04: interior wall decors missing, user suspects depth. CRITICAL CONTEXT: this is the same mechanism as kanban #29 (hut-interior wall decals — coincident faces in one OT bucket resolved by submission order), which is handled by RenderQueue::resolveKeyOrder. I rewrote resolveKeyOrder TODAY (psxport ffba3eaa) from pairwise enumeration to witness search. That change was gated by 5 tests pinning the snap set against a brute-force oracle, passing on BOTH sides — so it should be behaviour-preserving. BUT: equivalence to the PREVIOUS behaviour preserves a defect if the decals were already broken before it. Two distinct questions, answer both: (a) were the decors already missing BEFORE ffba3eaa? (b) does ffba3eaa change them at all? Check (a) first by testing a pre-ffba3eaa build; if they were already missing, ffba3eaa is exonerated and the cause is in the 07-28..07-31 window.
