---
id: 36
title: dev-warp: cold cross-area warp self-destructs ~50 frames later, and ids >=22 are accepted but are not areas
status: todo
labels: [bug, tooling]
created: 2026-07-22
updated: 2026-07-22
---

Two defects in the REPL/dbg 'warp <id>' dev tool, both found while closing #24. Neither is a port bug — the first reproduces IDENTICALLY on the oracle.

(A) COLD CROSS-AREA WARP SELF-DESTRUCTS. 'newgame; skip 60; warp N; skip 60' aborts for every N != current area with 'recomp-MISS 0x8010AC20 (caller ra=0x800587F8)'. 0x8010AC20 is jt[0] of the per-area table 0x800A45B8 (A00's handler), i.e. ActorTomba::enterOuterState0 read DAT_800BF870 == 0 while overlay A0N was resident, so an A00 address got dispatched into the wrong overlay. The warp writes bf870=N (it is in the log) and the game resets it to 0 within ~50 frames. PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1 hits the same abort at the same place with areaidx(800bf870)=0 in the miss-state dump — so it is the warp's forced transition sequence, not the native port. Controls: 'newgame; skip 300' (no warp) clean; 'warp 0' (same area) clean. Cold-warped areas also never finish their fade-in — every screenshot at +40..+45 frames is black. WORKAROUND that works today: settle the field first — 'newgame; skip 3000; warp N; skip 600; shot' renders every area 0..21 cleanly (this is what the 2026-07-22 layer sweep used). Likely cause to check first: the warp arms the door record (0x800BF839/0x800BF83A) and the field machine later re-runs that stale record, walking back toward area 0.

(B) 'warp <id>' ACCEPTS ids 0..31 BUT ONLY 0..21 ARE AREAS. Area load is FUN_80045080(0x80108F9C, area+3) = file index area+3 into the stride-8 LBA/size table at 0x800BE118. Dumped live that table is [0]=OPN [1]=CRD [2]=SOP [3..24]=A00..A0L [25]=START [26]=DEMO [27]=GAME, then zeros. So 'warp 22' loads START.BIN (1648 bytes) into the MODE slot, 'warp 23' loads DEMO.BIN, 'warp 25+' reads a zero descriptor — hence the '[disc] LBA … out of range' lines and the garbage stage (stage=0x90144C01). Corroborated three ways: exactly 22 A0*.BIN overlays on the disc; all six per-area MAIN tables are 22 entries; GAME.BIN's nexttab (0x80108F60) has [22]=[23]=0. FIX: reject id >= 22 in the warp command with a diagnostic instead of corrupting the mode slot. Files: external/psxport/runtime/recomp/repl.cpp (the 'warp' command) + native_boot.cpp (dev-warp area load).

Refs: docs/findings/scene.md (three new blocks), kanban #24.
