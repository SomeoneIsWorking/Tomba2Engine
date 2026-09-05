---
id: C027
kind: claim
status: holds
created: 2026-07-30
tags: libapi
---

## Claim

Guest 0x80086288 is libapi's VBlank INTERRUPT HANDLER (bump the libetc VSync tick counter at 0x800ABDE0, then call every non-null entry of the 8-slot VSyncCallback fn-ptr table at 0x800ABDC0) — an ordinary port, NOT PlatformHle material — and it is now natively owned by LibapiIntr::runVblankCallbacks and byte-exact to its guest-visible behavior.

## Evidence

IDENTITY, from the call sites: Ghidra headless (scratch/decomp/vsync_cb_chain_86288.c) shows its own immediate neighbour FUN_80086230 ending in func_0x80085b50(0, 0x80086288) = InterruptCallback(IRQ_VBLANK, handler), right after that same function zeroes the very table (clearWords(0x800abdc0, 8)) and the very counter (0x800abde0) this body walks and increments; Ghidra on 0x80086288 itself decompiles to iRam800abde0++ then a do-while over 0x800abdc0 calling each non-null slot. REACHABILITY: libsnd's SsSetTickMode parks it in the user-callback slot DAT_800AC430 and Sequencer::frameTick (0x800909C0) dispatches that slot per frame — confirmed live, the mirror gate reports entry ra=0x800909EC, the jal site inside guest 0x800909C0; ovhit counts native=800 over 400 frames. NOT PlatformHle: no spin, no completion-flag poll, no MMIO read, no call to the trapped VSync 0x80085900, bounded 8-iteration loop, and the guest-visible behavior already executed ~12k times a run without hitting a trap. BYTE-EXACTNESS: PSXPORT_MIRROR_VERIFY=0x80086288, 800 invocations, 0 mismatches over RAM+scratchpad+v0/v1/s0-s7/gp/sp/fp/ra+hi/lo (scratch/logs/mv_86288.log), with the instrument validated in both directions (I031). LIVE READ after 400 frames: [0x800ABDC0..DC] = all zero (no VSyncCallback installed), [0x800ABDE0] = 0x192, [0x800ABDE4] = 0x1F801114 (the Timer 1 MODE register, which initVblankCallbacks sets to 0x100 = count HBLANKs).

## What would falsify it

The byte-exactness half covers only the boot+title window: the 8 callback slots are all NULL there, so the indirect typed runtime address dispatch arm is UNEXERCISED. If any code ever installs a VSyncCallback, re-run the mirror gate — a divergence in the dispatch arm would falsify the byte-exactness claim without touching the identity claim. The identity claim is falsified if 0x80085B50 turns out not to be InterruptCallback, or if its first argument is not the IRQ index.
