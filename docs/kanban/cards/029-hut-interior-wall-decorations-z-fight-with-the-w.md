---
id: 29
title: Hut interior: wall decorations z-fight with the wall behind them
status: todo
labels: [bug, render]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22: in the hut interior the wall decorations z-fight against the wall they hang on. USER's own suggestion, and it is the right shape: 'we can use OT when things have the same depth'. That mechanism already exists — RenderQueue::resolveKeyOrder (kanban #11) enforces the GAME's own OT sort key wherever the depth buffer contradicts it. But it is currently scoped to face pairs WITHIN A SINGLE OBJECT (it stable_sorts the keyed faces by node and only compares inside a node), so a decoration and the wall — different nodes — are never considered. Generalising it to cross-object pairs whose depths are within a tolerance is the natural extension and uses the guest's own data, not a tag. Watch the failure mode #17 records: the interior-sample test is where the barrel fix is weak, and widening the scope widens that exposure too.
