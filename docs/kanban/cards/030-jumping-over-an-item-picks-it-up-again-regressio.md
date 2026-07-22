---
id: 30
title: Jumping over an item picks it up again — REGRESSION of #1
status: todo
labels: [bug, pc-skip]
created: 2026-07-22
updated: 2026-07-22
---

USER 2026-07-22: 'this jumping over items picks them issue is happening again also'. #1 was closed; it is back. Card #1 holds the original analysis and fix — read it first and find what undid it, rather than re-deriving. Prime suspects, both landed today: the recompiler jump-table fix (external c0caeef2) changed which code actually runs in four previously-shredded functions, and beh_prng_velocity_machine was rebuilt for readability + the delay-slot constant fix (f12ae00). The pickup trigger family is the obvious overlap with the latter.
