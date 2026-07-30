---
id: C024
kind: claim
status: holds
created: 2026-07-30
tags: ai
---

## Claim

kanban #8: the contact stamp never fires because the pump beams are never offered as candidates - the producer sees exactly ONE far-away item and correctly rejects it

## Evidence

MEASURED 2026-07-30 with a host-only counter inside the now-native producer, over 6,000+ calls on replays/bugs/seesaw-weight.pad (faithful path, PSXPORT_PC_SKIP=0): distinctItems=1 for the entire run. The single candidate is item=0x800FC8D8, radius 80, at XZ (9950,5700), while G=0x800E7E80 sits at (7227,3968) — a separation of (2723,1732), roughly 3,200 units against a radius of 80. So the overlap test 0x8002300C is CORRECT to return zero every time; nothing is broken inside the producer. The pump beams, whose nodes ents places at x=5562 and x=6678, are NEVER passed to it. This also puts the earlier aux-list observation back in play with the right role: the caller that would iterate MULTIPLE candidates is 0x801130C4, and its count at scratchpad 0x1F800144 is zero for the whole replay, so it contributes no calls; the 6,000 calls come from the other seven fixed-item callers. The claim C023 drew the wrong conclusion from that zero (that the producer is skipped) and remains falsified — the count does not gate the producer, it gates whether the beams are ever CANDIDATES.

## What would falsify it

a run where distinctItems exceeds 1, or where any candidate passed to 0x80111304 is within its radius of G, or evidence that a different producer stamps +0x2B for the beams
