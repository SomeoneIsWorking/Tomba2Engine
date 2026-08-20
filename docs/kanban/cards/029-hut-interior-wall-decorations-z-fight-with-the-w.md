---
id: 29
title: Hut interior: wall decorations z-fight with the wall behind them
status: done
labels: [bug, render]
created: 2026-07-22
updated: 2026-08-20
---

USER 2026-07-22: in the hut interior the wall decorations z-fight against the wall they hang on. USER's own suggestion, and it is the right shape: 'we can use OT when things have the same depth'. That mechanism already exists — RenderQueue::resolveKeyOrder (kanban #11) enforces the GAME's own OT sort key wherever the depth buffer contradicts it. But it is currently scoped to face pairs WITHIN A SINGLE OBJECT (it stable_sorts the keyed faces by node and only compares inside a node), so a decoration and the wall — different nodes — are never considered. Generalising it to cross-object pairs whose depths are within a tolerance is the natural extension and uses the guest's own data, not a tag. Watch the failure mode #17 records: the interior-sample test is where the barrel fix is weak, and widening the scope widens that exposure too.

The cross-object premise was falsified: the contesting pairs are the SAME node and SAME sort key, so
widening the contest would fix nothing. The 2026-07-22 equal-key snap was necessary but its tie premise
was wrong: native submission order is NOT the guest's intra-bucket walk order. PSX `AddPrim` inserts at
the bucket head, so submissions A,B become the walk B→A; native emits A→B. Equal D32 plus
GREATER_OR_EQUAL therefore picked the opposite final painter.

**2026-08-20 systemic correction:** `ot_lifo_depth.cpp` now assigns raster-distinct depths in AddPrim
LIFO order inside every selected same-key bucket and suppresses the renderer's generic later-native-draw
bias on those authored depths. It refuses rather than crossing the next nearer key band. Exact HUT replay
f1200: pixel (140,80) changed Native `(32,32,16)` → `(24,64,64)`, exactly GTE; 2,657/76,800 Native pixels
changed and the paired hanging decoration is visible. Second viewpoint (`left` + 80) is clean. Barrel #11
remains `(40,152,248)` on all 12 interpolated and 12 real presents; five HUT interp/real pairs are
0/76,800. The focused queue test models repeated AddPrim head insertion over 17 tied faces and requires
strictly reversed, raster-distinct D32 order.
