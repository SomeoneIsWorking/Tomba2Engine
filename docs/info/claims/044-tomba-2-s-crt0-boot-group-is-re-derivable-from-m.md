---
id: C044
kind: claim
status: holds
created: 2026-08-12
tags: boot,crt0,heap
depends: external/psxport/tools/crt0_extract.cpp
reconfirmed: 2026-08-13 04:18:00
verified_at: 2026-08-13 04:18:00
---

## Claim

Tomba!2's crt0 boot group is re-derivable from MAIN.EXE's own instruction stream — all 8 fields plus stackBias -8 and both heap globals match the values game_config.cpp recorded by hand

## Evidence

Measured 2026-08-12 by running the framework's own extractor over this repo's extracted boot executable: `$PSX/psxport/build/tools/crt0_extract scratch/bin/tomba2/MAIN.EXE` -> exit 0, '35 instruction(s) decoded, stopped on "jal (libcInit)", 3 zero word(s) in the window, prologue COMPLETE (reached the jal)', '8 of 8 field(s) resolved'. Derived: bssZeroLo 0x800BE0D8, bssZeroHi 0x80106228, stackTopBase 0x800A3F88, stackBias -8, stackTopBase2 0x800A3F8C, heapBase 0x80106228, gp 0x800BE0D4, libcInit 0x80089860, heapSizePtr 0x800ABEF8, heapBasePtr 0x800ABEF4, and 'libcInit is the A(39h) InitHeap BIOS thunk: YES . a1 live at the call: YES . delay slot is addi a0,a0,4: YES'. Every one of those ten addresses equals what game/core/game_config.cpp:27-35,187 already shipped, recorded independently BY HAND long before the extractor existed — so this is a two-source agreement, not a tool echoing a table. Derived heap: stack-top word mem[0x800A3F88]=0x00200000, bias -8 => sp=fp=0x801FFFF8; reserve word mem[0x800A3F8C]=0x400; heap size (0x1FFFF8-0x400)-0x106228 = 0xF99D0 = 1,022,416 bytes, comfortably under the framework's 0x800000 refusal ceiling. As of psxport 726d10c9 the SAME derivation (crt0_scan) runs inside the port at every boot (crt0_verify.h::crt0_audit) and refuses a confirmed disagreement, so extractor and boot gate cannot drift.

## What would falsify it

crt0_extract resolving fewer than 8 fields, reporting a prologue that is not COMPLETE, or reporting any address that differs from game_config.cpp on the same MAIN.EXE (sha-identity of the extracted file is NOT checked by this claim — a different MAIN.EXE would falsify it silently). Also falsified if crt0_audit ever refuses a boot: that would mean the static derivation and the running guest disagree. This claim is STATIC — it says nothing about a runtime value beyond what the boot-time audit checks.

## Re-confirmed 2026-08-12 21:13:21

RE-VERIFIED INDEPENDENTLY 2026-08-12 by a second session, on its own build (gate binary md5 32fcc50002dd, NOT the 4e21d132 build the original evidence used) and its own extractor invocation. (1) STATIC: $PSX/psxport/build/tools/crt0_extract scratch/bin/tomba2/MAIN.EXE (md5 31f07b2bee1fbb685cbc4c0afecc6f89) -> exit 0, "35 instruction(s) decoded, stopped on \"jal (libcInit)\", 3 zero word(s) in the window, prologue COMPLETE (reached the jal)", "8 of 8 field(s) resolved", and all ten addresses reproduce byte-for-byte: bssZeroLo 0x800BE0D8, bssZeroHi 0x80106228, stackTopBase 0x800A3F88, stackBias -8, stackTopBase2 0x800A3F8C, heapBase 0x80106228, gp 0x800BE0D4, libcInit 0x80089860, heapSizePtr 0x800ABEF8, heapBasePtr 0x800ABEF4, plus "libcInit is the A(39h) InitHeap BIOS thunk: YES . a1 live at the call: YES . delay slot is addi a0,a0,4: YES". Two-source agreement re-checked against game/core/game_config.cpp:27-35 (.bssZeroLo 0x800be0d8 .bssZeroHi 0x80106228 .stackTopBase 0x800a3f88 .stackTopBase2 0x800a3f8c .heapBase 0x80106228 .heapSizePtr 0x800abef8 .heapBasePtr 0x800abef4 .gp 0x800be0d4 .libcInit 0x80089860) and :187 (.stackBias = {1, -8}) -- 10 of 10 equal. (2) RUNTIME, the half the static tool cannot give: the in-port audit fired on a real headless boot and printed "guest-crt0 AUDIT at 0x800896E0 -- 35 instruction(s) decoded (3 zero word(s)), stopped at jal (libcInit): 10 field(s) AGREE, 0 DISAGREE, 0 unresolved" (scratch/logs/gate-boot-20260812-210921.log:50). So extractor, hand-recorded table and running guest all three agree.

## Re-confirmed 2026-08-12 21:34:38

THIRD independent verification 2026-08-12 (operator round), on binary md5 32fcc50002dd — reproduces in BOTH halves and the DEPENDS SET IS CORRECTED. (1) STATIC: $PSX/psxport/build/tools/crt0_extract over scratch/bin/tomba2/MAIN.EXE (md5 31f07b2bee1fbb685cbc4c0afecc6f89) -> rc 0, "35 instruction(s) decoded, stopped on \"jal (libcInit)\", 3 zero word(s) in the window, prologue COMPLETE (reached the jal)", "8 of 8 field(s) resolved". All ten values reproduce byte-for-byte and equal game/core/game_config.cpp:27-35,187 (10 of 10): bssZeroLo 0x800BE0D8, bssZeroHi 0x80106228, stackTopBase 0x800A3F88, stackBias -8, stackTopBase2 0x800A3F8C, heapBase 0x80106228, gp 0x800BE0D4, libcInit 0x80089860, heapSizePtr 0x800ABEF8, heapBasePtr 0x800ABEF4, plus "libcInit is the A(39h) InitHeap BIOS thunk: YES . a1 live at the call: YES . delay slot is addi a0,a0,4: YES". NOTE ON THE INSTRUMENT: that crt0_extract binary was built 21:21 from a psxport working tree with tools/crt0_extract.cpp MODIFIED (+284/-83, another agent in flight), so this is agreement across TWO different extractor builds, not one. (2) RUNTIME: PSXPORT_NOPACE=1 tools/gate.py boot --frames 400 -> PASS, scratch/logs/gate-boot-20260812-213146.log:50 "guest-crt0 AUDIT at 0x800896E0 - 35 instruction(s) decoded (3 zero word(s)), stopped at jal (libcInit): 10 field(s) AGREE, 0 DISAGREE, 0 unresolved". CORRECTION LANDED: external/psxport/tools/crt0_extract.cpp is now in depends: - it is the instrument for the STATIC half and was previously invisible to claim check, so a rewrite of the extractor could not have marked this claim stale.

## Re-confirmed 2026-08-13 04:18:00

Re-verified 2026-08-13 by a tool that shares no code with the one that first measured it: psxport tools/crt0_extract over MAIN.EXE resolves 8 of 8 fields with a COMPLETE prologue and agrees with game_config.cpp on all nine hand-recorded values (bssZeroLo 0x800BE0D8, bssZeroHi 0x80106228, stackTopBase 0x800A3F88, stackTopBase2 0x800A3F8C, heapBase 0x80106228, heapSizePtr 0x800ABEF8, heapBasePtr 0x800ABEF4, gp 0x800BE0D4, libcInit 0x80089860) plus stackBias -8 and crt0 entry 0x800896E0. Zero disagreements. Confirmed a third way at runtime: crt0_audit re-derives the group from the guest's own instruction stream on every boot and reported '10 field(s) AGREE, 0 DISAGREE, 0 unresolved' on my gate run at pin 553f0929.
