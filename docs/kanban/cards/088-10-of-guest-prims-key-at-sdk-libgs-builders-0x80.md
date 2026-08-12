---
id: 88
title: ~10% of guest prims key at SDK libgs builders (0x80080000/0x8008007C) — no frame on their chain is a producer claim
status: done
labels: [bug, render]
created: 2026-08-12
updated: 2026-08-12
---

2026-08-12, after the claim-resolution join (psxport 90604e18). otchain shows these chains (e.g. 0x8007FDB0 <- 0x800803DC <- 0x8003F698 <- 0x8003CDD8 <- 0x8003CCA4 <- 0x801092B4 <- 0x80109450) contain NO claimed frame within the 8-frame window, so the prim keeps its emitter key — which is an SDK packet builder, i.e. a row that names the library rather than the effect. 33,699 + 15,528 hits on a 300-frame run. Two candidate causes, neither measured: (a) the real handler for these prims genuinely has no native producer yet, in which case the row is honest but badly NAMED; (b) the handler is further than 8 frames out, which claimAtLimit would NOT catch because nothing within the window matched. Distinguish by re-running otchain with a wider window and diffing the verdict.

**2026-08-12:** 2026-08-12 PREMISE REFUTED, cause SETTLED — see the psxport commit 21676ab6 correction. (1) These are NOT SDK libgs builders: 0x8007FDB0 is the game's own POLY_GT3 submit leaf, 0x8008007C the POLY_GT4 leaf, 0x80080000 the OT-insert continuation of 0x8007FDB0 sharing its 24-byte frame. All three are already NATIVE-OWNED as ov_submit_poly_gt3/gt4 — game/render/submit.cpp:59 names all three explicitly. They only LOOK like SDK because they sit in the 0x80080000-0x8009E000 band that sync_overrides.cpp calls the SCEI library window. (2) Cause is (a), decisively, and (b) is dead: NO frame anywhere on either chain is one of the 9 claims — not merely outside the 8-frame window but out to the root at depth 28 — so widening CLAIM_SEARCH_DEPTH would change nothing. (3) 'No native producer' here does NOT mean 'not ported': the native code exists, only a ProducerScope claim on THIS route is missing. (4) THE FIX: claim the per-mode guest emitter the command routes to — 0x800803DC, the generic GT3/GT4 case — per the port's own keying rule in game/render/render_walk.cpp:101-119; never the shared dispatcher (it would shadow all eleven per-mode emitters into one meaningless row) and never a leaf.

**2026-08-12:** **CLOSED 2026-08-12 — the symptom is gone and the named fix is IN EFFECT, measured over every persisted run.**

MEASURED, with the denominator: 160 run jsonl files under scratch/producers/, 39 of them carrying any guest-keyed prims. Rows keyed at the three addresses this card was opened about — 0x8007FDB0, 0x8008007C, 0x80080000 — appear **0 times across all 160**. And the prims those chains carried now land on a NAMED per-mode emitter instead: run-2026-08-12T11:48:15.jsonl has 0x800803DC at 107,066 guest prims. 0x800803DC has its own curated row doc (docs/producers/0x800803DC.md, created by 9c94008), so the row now names the generic GT3/GT4 emitter rather than naming a library.

The mechanism that did it is the claim set, not a new ProducerScope: 0x800803DC entered scratch/producers/claims.txt (earned on a native leg) and the guest leg's resolveClaimedFrame now walks outward from the emitter frame to it. So the card's own recorded 'THE FIX: add a ProducerScope on 0x800803DC' was the wrong prescription for the right diagnosis — the scope already existed; what was missing was claim-set COVERAGE. The earlier correction in this card (these are the game's own POLY_GT3/GT4 submit leaves, already native-owned per game/render/submit.cpp:59, not SDK libgs builders) stands.

BLIND SPOT, stated rather than hidden: those 160 files are the runs that happened to persist, and this card's original 33,699+15,528 figure came from an otchain log, not a jsonl. So '0 leaf rows in 160 runs' is strong evidence the fix holds NOW; it is not proof the old counts were ever present in these particular files.

SPUN OUT, do not lose it: chasing this surfaced a separate defect in the claim set itself — see the new card on append-only claims fossilising after a keying change.
