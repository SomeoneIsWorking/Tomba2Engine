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

## Adjudicated 2026-08-06 — NOT RE-VERIFIED, and deliberately left `holds` rather than refreshed

The staleness flag is real but I could not answer it: the claim's numbers come from an
`overrides::coverage` line printed by a live SBS-full gate run, and running that gate needs the disc
image and a full build, neither of which this pass had. Refreshing the timestamp without the run
would convert a real signal into a lie, so the baseline is left where it is.

WHAT I DID CHECK, statically:
  - the instrument still exists — `overrides::coverage(int* total, int* unreached)` is declared at
    external/psxport/runtime/psx/override_registry.h:83 and defined at override_registry.cpp:167,
    so the measurement is still takeable.
  - the DENOMINATOR has moved. The claim's `411 owned addresses` dates from 2026-07-23;
    tools/codemap.py now counts 472 installed addresses over 312 files. That is a different counter
    from `overrides::coverage`'s own total, so it does not falsify 411 — but it does mean the ratio
    236/411 cannot be assumed to be today's ratio.

STATUS OF THE TWO HALVES: the structural half ("a green SBS run is silent about what the window
never reaches") is untested here and uncontradicted by anything found. The NUMERIC half (236/411,
~43%, and the 273/411 in the 2026-07-23 reconfirm) should be treated as DATED, not current.
NEXT: re-run the driven gate route `replays/gate/seaside-sweep.sbskeys` and read the coverage line.
