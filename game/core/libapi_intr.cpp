// game/core/libapi_intr.cpp — see libapi_intr.h for scope and for why this is not PlatformHle.
//
// RE: Ghidra headless on the existing project (scratch/decomp/gpu_dma_85c9c.c, FUN_80085c9c together
// with its caller FUN_80082d04) cross-checked against generated/shard_0.c gen_func_80085C9C, which is
// four instructions:
//
//     r3 = mem_r32(0x800ABDA8)          // libapi's hardware-pointer table entry 0 = I_MASK
//     r2 = mem_r16(r3 + 0)              // previous mask, ZERO-extended (lhu) -> v0
//     mem_w16(r3 + 0, (u16)a0)          // store the new mask
//
// The identification rests on the CALLER, not on the shape: Ghidra shows FUN_80082d04 bracketing its
// DMA-ring manipulation with `saved = f(0); ... ; f(saved)`, the save/disable/restore idiom, and a
// live REPL read confirms the table entry is 0x1F801074. A four-instruction load-store swap could
// otherwise have been almost anything.
//
// Note the 16-bit read is UNSIGNED (lhu) while the pointer load is a full word — mem_r16 and
// mem_r32 respectively. Using mem_r16s here would sign-extend a mask with bit 15 set into a
// 0xFFFFxxxx return value.
#include "core.h"
#include "game.h"
#include "libapi_intr.h"
#include "override_registry.h"   // engine_set_override_main
#include "rec_decls.h"           // gen_func_80085C9C — the body the oracle leg runs

namespace {
// libapi's hardware-register pointer table. Entry 0 is I_MASK (0x1F801074); entry 1 is DPCR
// (0x1F8010F0). Only entry 0 is reached from here.
constexpr uint32_t kLibapiHwPtrTable = 0x800ABDA8u;
}  // namespace

void LibapiIntr::setIntrMask(Core* c) {
  const uint32_t imaskPtr = c->mem_r32(kLibapiHwPtrTable);
  const uint32_t previous = c->mem_r16(imaskPtr);      // lhu — zero-extended, see the banner
  c->mem_w16(imaskPtr, (uint16_t)c->r[4]);
  c->r[2] = previous;
}

void LibapiIntr::registerOverrides(Game*) {
  engine_set_override_main(0x80085C9Cu, &LibapiIntr::setIntrMask, gen_func_80085C9C);
}
