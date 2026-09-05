---
id: C020
kind: claim
status: holds
created: 2026-07-29
tags: re
---

## Claim

A wide-RE BANKED DRAFT can be confidently wrong in a way its own banner denies — re-verify every draft against its guest-visible behavior line by line before wiring it

## Evidence

guest 0x80083DE0's draft (game/render/wide_re_libgpu_leaves.cpp) asserted 'a3(r7)=UNUSED by this leaf (register alias only, verified: the guest-visible behavior never reads r7)'. authenticated executable/overlay evidence reads r7 — it BRANCHES on a1 and MASKS a3. The draft used a1 for both. Corrected + wired 2026-07-29; once a3 is the tpage the signature is exactly stock libgpu SetDrawMode(p,dfe,dtd,tpage,tw).

## What would falsify it

a systematic re-verify of the remaining banked drafts finding them all faithful, which would make this a one-off rather than a property of the bank

## Adjudicated 2026-08-06 — NOT RE-VERIFIED; the staleness flag is a FALSE POSITIVE by construction

This is a DATED OBSERVATION, not a statement about current code: "guest 0x80083DE0's draft asserted X;
the guest-visible behavior says Y; corrected + wired 2026-07-29". The commit that flagged it stale (d831ce6) is
the commit that FIXED the defect it records. Fixing the observed defect cannot falsify the
observation — a retrospective claim's evidence is in history, so `depends:`-style rot detection,
which asks "has the cited file changed since?", mis-flags it every time that file is touched again.

Its OWN falsifier is the right test and it has NOT been run: "a systematic re-verify of the
remaining banked drafts finding them all faithful". I re-verified no drafts in this pass. Left
`holds`; not confirmed either, because confirming would imply a re-sample that did not happen.

TOOL DEFECT this exposes (shared with C021): tools/info.py has no way to mark a claim as a dated
observation, so every such claim generates permanent staleness noise that trains readers to ignore
the signal. Worth fixing at the tool, not by editing this file every month.
