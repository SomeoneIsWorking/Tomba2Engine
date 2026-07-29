// game/core/libapi_intr.h — the kernel INTERRUPT-MASK primitives (SCEI libapi).
//
// Scope is deliberately narrow: the leaves that read or write the PSX interrupt-control registers
// through libapi's hardware-pointer table at 0x800ABDA8. That table is not a guess — a live read at
// f300 shows [0x800ABDA8] = 0x1F801074 (I_MASK) and [0x800ABDAC] = 0x1F8010F0 (DPCR), i.e. libapi's
// own array of hardware register addresses.
//
// This is NOT PlatformHle material. PlatformHle exists for primitives whose real bodies BUSY-SPIN on
// a hardware IRQ our no-IRQ runtime never raises, and which therefore must be replaced rather than
// reproduced. SetIntrMask does not spin: it is a plain load-store through that table, so the native
// body is a faithful mirror and goes through the ordinary override registry like any other leaf,
// oracle-gated so core B keeps running the recompiled body.
#pragma once
struct Core;
class  Game;

class LibapiIntr {
public:
  // FUN_80085C9C — SetIntrMask(u16 mask) -> u16 previousMask. Reads the current 16-bit value at
  // *0x800ABDA8 (= I_MASK), stores the new one, returns the old, zero-extended.
  //
  // Callers use it as the standard save/disable/restore bracket around a critical section, which is
  // what identifies it. From the GPU-DMA enqueue path (Ghidra, FUN_80082D04):
  //     saved = SetIntrMask(0);   ... touch the DMA ring ...   SetIntrMask(saved);
  static void setIntrMask(Core* c);

  static void registerOverrides(Game* game);
};
