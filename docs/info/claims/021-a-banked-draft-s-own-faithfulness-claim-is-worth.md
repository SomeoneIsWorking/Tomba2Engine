---
id: C021
kind: claim
status: holds
created: 2026-07-29
tags: re
---

## Claim

A banked draft's own faithfulness claim is worth nothing — of five drafts checked on 2026-07-29, three were defective, and every defect was invisible locally

## Evidence

libgpuSetDrawMode (game/render/wide_re_libgpu_leaves.cpp): masked a1 where the gen body masks a3, while its banner asserted 'the gen body never reads r7'. beh_substate_edge_leaves.cpp (all four drafts): descend sp and write NONE of their 22 guest stack spills, stashing registers in C++ locals. Timing::vsyncCallbackDispatch (external/psxport/runtime/recomp/timing.cpp): data addresses 0x4000 too high AND r16/r17 spills swapped, under a banner claiming faithfulness to gen_func_80086288. FAITHFUL on check: libgpuDmaStatusReset and GteTransform3::rotate3AndPackIr. Every defect was self-consistent — the code restores what it saved, so nothing looks wrong without diffing the gen body.

## What would falsify it

a sample of ten or more further drafts checked line-by-line against their gen bodies coming back clean, which would make 3-of-5 an unlucky draw rather than the base rate

## Adjudicated 2026-08-06 — NOT RE-VERIFIED; same false-positive shape as C020

"Of five drafts checked on 2026-07-29, three were defective" is a historical sample. All three
commits that flagged it stale (2e2c8b3, b558c5e, 3f25af8a) are the commits that REPLACED or DELETED
the defective drafts it names — i.e. the claim's own recommended action being carried out. That is
the opposite of falsification.

Its falsifier — "ten or more further drafts checked line-by-line against their gen bodies coming
back clean" — has NOT been run in this pass, so the 3-of-5 base rate is neither reconfirmed nor
overturned. Left `holds` and explicitly NOT timestamp-refreshed. See C020 for the tool defect.
