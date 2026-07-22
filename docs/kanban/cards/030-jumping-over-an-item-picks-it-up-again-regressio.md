---
id: 30
title: Jumping over an item picks it up again — REGRESSION of #1
status: done
labels: [bug, pc-skip]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22: 'this jumping over items picks them issue is happening again also'. #1 was closed; it is back. Card #1 holds the original analysis and fix — read it first and find what undid it, rather than re-deriving. Prime suspects, both landed today: the recompiler jump-table fix (external c0caeef2) changed which code actually runs in four previously-shredded functions, and beh_prng_velocity_machine was rebuilt for readability + the delay-slot constant fix (f12ae00). The pickup trigger family is the obvious overlap with the latter.

**2026-07-22:** NOT A REGRESSION — root-caused 2026-07-22. ActorTomba::proximityCheck (FUN_80022060) sign-extended both accept gates that the guest zero-extends (andi 0xffff at 0x800220E0 distance / 0x80022118 vertical band). Y is down-positive, so clearing an item by more than upT+upI makes the vertical sum negative; the guest's andi turns that into ~0xFF00 and REJECTS, the port read it as a small negative and ACCEPTED — every jump-over collects. Code unchanged since the repo import, so #1 was masked (by the #2 softlock), never fixed; f12ae00 unmasked it. Neither prime suspect is implicated: beh_prng_velocity_machine is byte-identical to its gen body (A/B on bucket-softlock.pad, cmp=0). Fixed by zero-extending both gates; contact pickup still works (bucket replay still collects), RAM byte-identical where the defect never fired, SBS unchanged. GAP: symptom not reproduced headless — cause proven from the instruction stream + measured jump height.
