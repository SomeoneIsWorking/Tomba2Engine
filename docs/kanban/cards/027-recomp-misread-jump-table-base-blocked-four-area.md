---
id: 27
title: recomp: misread jump-table base blocked four areas (10/11/13/14) — FIXED
status: done
labels: [recomp]
created: 2026-07-22
updated: 2026-07-22
---

2026-07-22. Areas 10, 11, 13 and 14 could not be entered at all: each aborted with a rec_dispatch miss (0x80028FA4 / 0x80028FC4 / 0x80029004 / 0x80029024) from FUN_80028E10, the object-template init dispatcher.

ROOT CAUSE (external/psxport/tools/recomp/emit.py). The strict switch-idiom scan cannot model a table base built through a second register (`lui tmp,HI ; addiu base,tmp,LO`). Rather than failing, it kept scanning and matched the `lui` belonging to one of the OTHER base candidates. MAIN 0x8003BBF8 is that shape — `lui v0,0x8001 ; addiu s3,v0,0x4A70 ; addu v0,v0,s3 ; lw v0,0(v0) ; jr v0` — so the base resolved to 0x80010000 (the lui alone; the real base is 0x80014A70) and 144 'case labels' were read out of .text's head. That run swallowed BOTH the genuine 22-entry area-mode stub table at 0x80010000 AND the head of FUN_80028E10's own switch table at 0x8001021C. The cross-boundary seeder promoted 9 of the overlapping labels to function entries, truncating FUN_80028E10 at 0x80028E64 so its switch degenerated to `default: rec_dispatch(target)` — and the 12 cases nobody had accidentally seeded aborted the moment an area used them.

FIX: strict bails out on the two-register base form so the ENHANCED pass resolves the real base. MAIN function set 2207 -> 2119; the 99 addresses that stop being functions are all mid-function case labels and their 10 containers now recover their own switches. The area-mode stubs 0x8001CB00..0x8001CB98 are seeded explicitly (Engine::areaModeDispatchFaithful dispatches 0x8001CB98 by address). RECOMP_VERSION bumped to 2026-07-22.1.

Also removed: leaf_80024E00 / leaf_80025744 / leaf_80028E10 in game/core/field_owned_leaves.cpp — register-literal transcriptions of the OLD truncated bodies, carrying the same `default: rec_dispatch` hole. They were ORPHANs (0x80025744 was additionally dual-owned with the real Render::fieldHudStatusRow), so deleting them lets the now-correct gen bodies run.

VERIFIED: areas 10/11/13/14 load and render (scratch/screenshots/layergap/y10..y14); seaside f6000 pc-vs-psx unchanged at 90 renderer-attributable px before and after; SBS full 0-diff through f18870.
