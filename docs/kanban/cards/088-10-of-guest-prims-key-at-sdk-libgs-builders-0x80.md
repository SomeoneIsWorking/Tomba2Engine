---
id: 88
title: ~10% of guest prims key at SDK libgs builders (0x80080000/0x8008007C) — no frame on their chain is a producer claim
status: todo
labels: [bug, render]
created: 2026-08-12
updated: 2026-08-12
---

2026-08-12, after the claim-resolution join (psxport 90604e18). otchain shows these chains (e.g. 0x8007FDB0 <- 0x800803DC <- 0x8003F698 <- 0x8003CDD8 <- 0x8003CCA4 <- 0x801092B4 <- 0x80109450) contain NO claimed frame within the 8-frame window, so the prim keeps its emitter key — which is an SDK packet builder, i.e. a row that names the library rather than the effect. 33,699 + 15,528 hits on a 300-frame run. Two candidate causes, neither measured: (a) the real handler for these prims genuinely has no native producer yet, in which case the row is honest but badly NAMED; (b) the handler is further than 8 frames out, which claimAtLimit would NOT catch because nothing within the window matched. Distinguish by re-running otchain with a wider window and diffing the verdict.

**2026-08-12:** 2026-08-12 PREMISE REFUTED, cause SETTLED — see the psxport commit 21676ab6 correction. (1) These are NOT SDK libgs builders: 0x8007FDB0 is the game's own POLY_GT3 submit leaf, 0x8008007C the POLY_GT4 leaf, 0x80080000 the OT-insert continuation of 0x8007FDB0 sharing its 24-byte frame. All three are already NATIVE-OWNED as ov_submit_poly_gt3/gt4 — game/render/submit.cpp:59 names all three explicitly. They only LOOK like SDK because they sit in the 0x80080000-0x8009E000 band that sync_overrides.cpp calls the SCEI library window. (2) Cause is (a), decisively, and (b) is dead: NO frame anywhere on either chain is one of the 9 claims — not merely outside the 8-frame window but out to the root at depth 28 — so widening CLAIM_SEARCH_DEPTH would change nothing. (3) 'No native producer' here does NOT mean 'not ported': the native code exists, only a ProducerScope claim on THIS route is missing. (4) THE FIX: claim the per-mode guest emitter the command routes to — 0x800803DC, the generic GT3/GT4 case — per the port's own keying rule in game/render/render_walk.cpp:101-119; never the shared dispatcher (it would shadow all eleven per-mode emitters into one meaningless row) and never a leaf.
