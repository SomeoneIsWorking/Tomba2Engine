---
id: 85
title: Producer DB guest leg keys the EMITTER frame while native rows key the HANDLER — 2 of 25 keys coincide, every guest row reads native 0
status: todo
labels: [bug,render]
created: 2026-08-12
updated: 2026-08-12
---

Measured 2026-08-12 on a 300-frame house-on-the-point run after psxport 38cec620 gave the guest leg identity (span-no-fn 274089 -> 0, attributed 284193). otattrTop() names the INNERMOST emitter frame (0x8003DF04 OverlayGt3Gt4, 0x801465EC/0x801467BC ground gt3/gt4, 0x8013FE58/0x8013FB88 backdrop) while native rows are keyed at the handler/pass frame (0x80146478 perObjFlush, 0x80109FE0 fieldEntityRender, 0x8002AB5C terrainRender). Result: the two legs land in DISJOINT rows, so the DB looks complete while comparing nothing — this is the blocker on the GTE-vs-native comparison the user asked for. ~10% of guest keys are SDK libgs builders (0x80080000 14455, 0x8008007C, 0x8007FDB0). The maintained stack holds BOTH frames because indirect dispatch also goes through the wrapper, so the census can key on 'the frame in the chain that a native producer/DB row claims' instead of the top. NOT implemented, NOT measured. See docs/findings/render.md.
