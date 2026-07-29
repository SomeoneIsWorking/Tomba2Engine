---
id: C020
kind: claim
status: holds
created: 2026-07-29
tags: re
---

## Claim

A wide-RE BANKED DRAFT can be confidently wrong in a way its own banner denies — re-verify every draft against its gen body line by line before wiring it

## Evidence

func_80083DE0's draft (game/render/wide_re_libgpu_leaves.cpp) asserted 'a3(r7)=UNUSED by this leaf (register alias only, verified: the gen body never reads r7)'. generated/shard_0.c:12643 reads r7 — it BRANCHES on a1 and MASKS a3. The draft used a1 for both. Corrected + wired 2026-07-29; once a3 is the tpage the signature is exactly stock libgpu SetDrawMode(p,dfe,dtd,tpage,tw).

## What would falsify it

a systematic re-verify of the remaining banked drafts finding them all faithful, which would make this a one-off rather than a property of the bank
