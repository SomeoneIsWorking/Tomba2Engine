---
id: 88
title: ~10% of guest prims key at SDK libgs builders (0x80080000/0x8008007C) — no frame on their chain is a producer claim
status: todo
labels: [bug,render]
created: 2026-08-12
updated: 2026-08-12
---

2026-08-12, after the claim-resolution join (psxport 90604e18). otchain shows these chains (e.g. 0x8007FDB0 <- 0x800803DC <- 0x8003F698 <- 0x8003CDD8 <- 0x8003CCA4 <- 0x801092B4 <- 0x80109450) contain NO claimed frame within the 8-frame window, so the prim keeps its emitter key — which is an SDK packet builder, i.e. a row that names the library rather than the effect. 33,699 + 15,528 hits on a 300-frame run. Two candidate causes, neither measured: (a) the real handler for these prims genuinely has no native producer yet, in which case the row is honest but badly NAMED; (b) the handler is further than 8 frames out, which claimAtLimit would NOT catch because nothing within the window matched. Distinguish by re-running otchain with a wider window and diffing the verdict.
