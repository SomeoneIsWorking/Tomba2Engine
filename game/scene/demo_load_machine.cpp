#include "demo_load_machine.h"

#include "core.h"
#include "game.h"
#include "game_ctx.h"
#include "guest_call.h"
#include "native_override_catalog.h"

namespace tomba::demo {

void stepLoadMachine(Core &core) {
  auto *c = &core;
  const std::uint8_t state = c->mem_r8(0x800bf84au);
  switch (state) {
  case 0:
    psx::cpu::dispatchGuestToReturn0(
        *c, 0x8001cf2cu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__); // engine update
    { // FUN_80045558(1) = FUN_80045080(0x8018a000, idx=1) = FUN_8001dc40(dest, lba, size) — SYNC read
      const std::uint32_t table = 0x800be118u + 1u * 8u; // indexed file table, stride 8 {lba,size}
      native::retireOverlay(*c, eng(c).activeAreaOverlay);
      c->game->cd.dc40Sync(0x8018a000u, c->mem_r32(table), c->mem_r32(table + 4u));
      native::activateAreaSlotOverlay(*c, eng(c).activeAreaOverlay, 1u);
    }
    c->mem_w8(0x1f80019bu, 1); // mark the load complete (case 3 reads this)
    c->mem_w8(0x800bf84au, 1);
    c->r[4] = 0x0c;
    c->r[5] = 1;
    psx::cpu::dispatchGuestToReturn0(
        *c, 0x800750d8u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__); // FUN_800750d8(0xc, 1): page open
    break;
  case 1:
    c->r[4] = 0;
    psx::cpu::dispatchGuestToReturn0(
        *c, 0x8007be18u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__); // overlay slot browser (param2==0 path)
    if (c->mem_r16(c->mem_r32(kStatePtr) + 0x50) > 1) {
      c->mem_w8(0x800bf84au, 3); // user picked/cancelled -> poll done
    }
    break;
  case 2: // (param2==0 skips this; sync = always done)
    c->mem_w8(0x800bf84au, 3);
    break;
  case 3:
    if (c->mem_r8(0x1f80019bu) != 0) {
      c->mem_w8(0x800bf84au, 4);
    }
    break;
  case 4:
    c->r[4] = 0;
    psx::cpu::dispatchGuestToReturn0(
        *c, 0x8007be18u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__); // run the chosen action (sets sm[0x6b])
    c->mem_w8(0x800bf84au, 0);
    break;
  default:
    c->mem_w8(0x800bf84au, 0);
    break;
  }
}

} // namespace tomba::demo
