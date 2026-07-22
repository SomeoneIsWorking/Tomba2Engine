---
id: 29
title: Hut interior: wall decorations z-fight with the wall behind them
status: done
labels: [bug, render]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22: in the hut interior the wall decorations z-fight against the wall they hang on. USER's own suggestion, and it is the right shape: 'we can use OT when things have the same depth'. That mechanism already exists — RenderQueue::resolveKeyOrder (kanban #11) enforces the GAME's own OT sort key wherever the depth buffer contradicts it. But it is currently scoped to face pairs WITHIN A SINGLE OBJECT (it stable_sorts the keyed faces by node and only compares inside a node), so a decoration and the wall — different nodes — are never considered. Generalising it to cross-object pairs whose depths are within a tolerance is the natural extension and uses the guest's own data, not a tag. Watch the failure mode #17 records: the interior-sample test is where the barrel fix is weak, and widening the scope widens that exposure too.

**2026-07-22:** FIXED 2026-07-22. Premise falsified then fixed differently: the contesting pairs are SAME node AND SAME sort key (same OT bucket) — cross-object widening of resolveKeyOrder would have fixed nothing. Real cause: exactly-coincident decal+wall quads listed with ROTATED vertex order, so the fixed (0,1,2)+(1,2,3) triangulation splits them on opposite diagonals and the earlier-submitted face wins half the quad (115/541 px at f1200). Fix: resolveKeyOrder's equal-key branch now snaps EXACTLY coincident faces (identical vertex multiset incl. depth) to their shared key_ord, so submission order = the guest's intra-bucket link order decides. Verified: 281 px change at f1200, 0/281 matched the psx oracle before, 281/281 after; second viewpoint 0/214 -> 214/214; barrel #11 pixel (165,92) still (40,152,248); fps60 TFORCE interp-vs-real 0/76800 x10; 4 unrelated scenes 0 px changed. Headless repro recorded in replays/README.md (hut-entry-door-freeze.pad, no newgame, run 1200).
