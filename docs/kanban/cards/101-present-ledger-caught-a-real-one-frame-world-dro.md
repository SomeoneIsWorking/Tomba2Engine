---
id: 101
title: Present ledger caught a real one-frame world drop at the hut door transition (f390)
status: todo
labels: [bug,render]
created: 2026-08-16
updated: 2026-08-16
---

FOUND BY THE NEW GATE on its first run — tools/present_gate.py / present_ledger.h.

replays/scene-transitions/hut-entry-door-freeze.pad, frame 390, BOTH configs identically:
  [ledger] f389 OK      — captured 8   presented 1095  (bg 352/0 world 743/8)
  [ledger] f390 DROPPED — layer world  captured 426  presented 0   (totals: captured 426 presented 0)
  [ledger] f391 OK      — captured 539 presented 117

The real present emitted ZERO prims on f390 across every layer while 426 world prims sat in the
capture. One frame, at the door-transition boundary into the hut interior — i.e. exactly where
mTier1EligibleCur flips and the scene stops being field.

Likely shape (INFERRED, not yet measured): the frame is tier1-eligible so the merge skips the captured
world as tier1-owned, while tier1Render produces an empty sink because the field scene has already been
torn down — so the world is skipped from the capture AND absent from the sink. That is the kanban #50
class (stale/absent sink across a scene boundary), which the eligibility latch was introduced to handle.

Not blocking anything today: it is one frame inside a transition that also fades, so it is very likely
invisible in play. It is filed because the gate is only worth having if what it catches gets looked at,
and because it is the first evidence the ledger discriminates on real content rather than only in its
unit test.

NOTE: this is why tools/present_gate.py is NOT yet wired into tools/precommit_gate.sh — a gate that is
red on main teaches people to ignore it. Wire it in the same change that fixes this.
