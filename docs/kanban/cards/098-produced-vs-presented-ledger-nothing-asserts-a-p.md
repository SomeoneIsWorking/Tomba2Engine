---
id: 98
title: Produced-vs-presented ledger: nothing asserts a pushed prim reached the screen
status: todo
labels: [instrument,render,verification]
created: 2026-08-16
updated: 2026-08-16
---

The gap that let kanban #94/#35 ship, and that made the panel family read as endlessly re-broken.

The producer census answers 'which producer owns this prim that ARRIVED'. There is no counter for prims that never arrive: no abort/exit/assert path, and prims_native_max is monotonic-max-folded so a drop from N to 0 can never show. 'grep -c producers tools/precommit_gate.sh' = 0 — nothing in CI touches it. Measured: its run-end line read 'prims seen 1728103 = attributed 1708014 + unscoped-native 20089' — fully green — while an entire UI layer was missing from the screen.

DESIGN (from the #94 investigation):
- produced[key][layer] incremented at push (emitOrQueue/push2dQuad, the census's existing chokepoint).
- presented[key][layer] incremented in RenderQueue::emitItem (the single funnel both presentPass slots and the plain emitQueue path reach the GPU through).
- reconcile at the frame fence; fatal under PSXPORT_GATE_PRESENTATION=1, ERROR otherwise.

DESIGN THE NEGATIVE FIRST — three distinct outputs, and the third is the one that matters:
  OK line, with denominator: 'f930 OK - produced 663 = presented 663, across 2 flush(es); by layer world 642/642 overlay 9/9 hud 12/12'
  DROP line, naming the producer: 'f930 DROPPED 21 of 663 prim(s) between flush and present. layer overlay: produced 9 presented 0 producer 0x8004FFB4 Panel::pushFill ...'
  NEVER-FED line: 'the ledger was NEVER FED - 0 frames reconciled, 0 present(s) observed. This does NOT mean nothing was dropped; the counters may not be wired or this run never reached present_vk. This run proves NOTHING.'

Plus a --selftest in the SHIPPING artifact that captures twice without accumulating and asserts the ledger reports exactly DROPPED 3 — the instrument must be shown producing the positive verdict before it is trusted to produce negatives. Wire into ctest beside test_producer_census.cpp, and into precommit_gate.sh over the four panel replays at BOTH fps60=0 and fps60=1.

Why not pixels: needs no golden image, no oracle, no aspect/timing match, and fires on any scene the gate drives rather than an enumerated list. It would have failed the day the regression landed instead of weeks later.
