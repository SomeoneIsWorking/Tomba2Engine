---
id: 91
title: Producer claim set is append-only with NO provenance — a claim fossilises when a producer's key moves
status: todo
labels: [bug, producers, psxport, instrument]
created: 2026-08-12
updated: 2026-08-12
---

Found 2026-08-12 while closing #88, which it is NOT a duplicate of: #88 was prims keying at a submit leaf; this is the claim set being unable to retire a claim.

THE MECHANISM. psxport runtime/recomp/producer_census.h::appendClaims() appends unconditionally and its own comment states the rationale: 'append-only; a claim earned by a native producer drawing is never un-earned by a later leg that skips it'. That rationale is CORRECT for the case it names — a guest leg does not run native producers, so absence on one leg must not retire a claim. It does NOT cover the case that actually occurred: **the producer's KEY moved because the CODE changed.** Nothing in the file records which build earned which address, so a live claim and a fossil are indistinguishable.

THE OBSERVATION THAT EXPOSED IT. Two native-leg runs, both 298 frames, both exactly 152,981 prims — the same workload — keyed at DIFFERENT addresses:
  run-2026-08-11T23:57:38.jsonl -> 0x800803DC  prims_native 152981  frames 298
  run-2026-08-12T13:16:14.jsonl -> 0x80146478  prims_native 152981  frames 298
9c94008 ('key perObjFlush's world prims by the per-mode emitter', 2026-08-12 00:12:21) sits between them and is what moved the key: it introduced Render::resolvePerModeEmitter and keyed the scope on the resolved per-mode emitter, where 0x800803DC is only the GENERIC_EMITTER fallback (perobj_dispatch.cpp:118). Both addresses are legitimate producers with curated docs (docs/producers/0x800803DC.md, docs/producers/0x80146478.md), which is exactly why neither looks wrong in isolation.

WHY IT MATTERS — it manufactures a false negative, the worst kind here. claims.txt currently holds 106 lines for 10 distinct addresses (append-only, so one block per run). If an address in that set is no longer earnable by the present build, the guest leg still resolves to it — run-2026-08-12T11:48:15.jsonl resolves 107,066 guest prims to 0x800803DC with prims_native 0. A reader of the DB sees 'guest prims, no native producer' and concludes an effect is unported, when the native producer may exist under the key it moved to. That is the DB asserting a negative its method cannot support.

WHAT IS **NOT** ESTABLISHED, and must be measured before fixing: whether 0x800803DC is currently earnable by the present build at all, or is already a fossil. Both keys being reachable outcomes of one resolvePerModeEmitter switch means the honest answer needs a native run that reports WHICH addresses this build earned, diffed against the set on disk. Do not 'fix' this by pruning claims.txt until that diff exists — pruning a live claim would silently lose attribution.

FIX DIRECTION (framework, psxport — not a game change). Give each claim PROVENANCE and make staleness VISIBLE rather than pruning silently: record alongside each address the build identity and run stamp that earned it, and have producer_db_finish REPORT claims in the loaded set that no run has re-earned since a given build ('N of M claims in the set were last earned by a different build — attribution to those may be a fossil'). A loud stale-claim line preserves the append-only property that the existing comment correctly defends, while removing the silence that made this findable only by accident.
