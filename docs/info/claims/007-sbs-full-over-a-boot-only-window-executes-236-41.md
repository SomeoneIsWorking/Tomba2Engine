---
id: C007
kind: claim
status: holds
created: 2026-07-23
tags: 
reconfirmed: 2026-07-23
---

## Claim

SBS full over a boot-only window executes ~236/411 owned override addresses; ~43% of what we OWN is NEVER reached by the gate, so a green SBS run is silent about that 43%

## Evidence

overrides::coverage printed by the gate at exit: 'coverage: 236/411 owned addresses executed this run — 175 NEVER reached'; kanban #60 was a guaranteed A/B divergence that sat green because its opcode was outside the window

## What would falsify it

the coverage line showing unreached==0, or the gate being driven over a replay/scenario that reaches the currently-unreached addresses

## Re-confirmed 2026-07-23

driven gate route replays/gate/seaside-sweep.sbskeys raises coverage 236->273/411 (66%) byte-exact through f27500; residual 138 is content-wall (61 field_owned_leaves + other-area behaviours), needs traversal not a denser route
