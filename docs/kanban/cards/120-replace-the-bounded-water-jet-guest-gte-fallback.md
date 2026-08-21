---
id: 120
title: Replace the bounded water-jet guest-GTE fallback with a native producer
status: todo
labels: [render, debt]
created: 2026-08-21
updated: 2026-08-21
---

FUN_8013D454 non-zero-mode water-jet mesh is visible again through game/render/guest_gte_water_jet.cpp under the explicit 2026-08-21 user authorization for non-interpolated actual guest-GTE output. This is deliberately hack debt: the generated controller and FUN_80027768 writer remain authoritative; only their newly written exact GT4 packets are replayed at logic time, and every packet must resolve 4/4 address-keyed guest depths or abort. True-software-oracle B is byte-identical at f450/460/470/480/490/500/510/520; f450/f500 are exact no-call controls; the landed #15 impact replay remains exact in both panes. Proper completion requires a controller-state display producer for both non-zero modes from node+0x60, table 0x8010A058, node angles/anchor and shared mesh records, then deleting the fallback. Never widen it to another FUN_80027768 caller. Evidence: docs/findings/render.md bounded 0x8013D454 section and scratch/logs/waterjet_final.log.
