#include "tomba_runtime.h"

#include "core.h"
#include "engine.h"
#include "game.h"
#include "game_ctx.h"
#include "guest_call.h"
#include "legacy_game_interface.h"
#include "register_overrides.h"

#include <lucent/log.h>

namespace tomba {

TombaRuntime::TombaRuntime() : LegacyGameRuntimeAdapter(legacy::measuredConfig, legacy::compatibilityHooks) {}

void *TombaRuntime::createContext(Core &core) {
  return createTombaContext(core);
}

void TombaRuntime::destroyContext(void *context) {
  destroyTombaContext(context);
}

void TombaRuntime::registerOverrides(Game &game) {
  register_engine_overrides(game);
}

bool TombaRuntime::guestVramIsPicture(const Game &) const {
  // The native render pass owns the complete presented frame. Guest VRAM still carries texture
  // atlases and compatibility-path drawing, but neither is background picture content for the
  // native compositor. Keep this title policy on the derived runtime instead of routing a renderer
  // decision through the transitional flat GameConfig view.
  return false;
}

void TombaRuntime::bootInit(Core &core) {
  Core *c = &core;
  lucent::info("native_boot", "FUN_80050b08 override: running init prefix");

  // FUN_80050b08's init prefix, without its scheduler loop. The guest calls and native engine
  // owners are deliberately kept in retail order because task0Bootstrap depends on the scheduler
  // table and current-task writes between them.
  rc0(c, 0x80089788);
  rc0(c, 0x80085b20);
  // The guest CdInit busy-waits on a controller-reset IRQ that this native host does not model.
  // Every disc operation is synchronous and native, so initialize that owner directly.
  c->game->cd.hleInit();
  rc1(c, 0x80080bf0, 3);
  rc1(c, 0x80080d64, 0);
  rc1(c, 0x80080ed4, 1);
  rc1(c, 0x800865f0, 0);
  eng(c).initFrameState(); // was FUN_80050a0c
  eng(c).initDisplay();    // was FUN_800509b4; establishes projection H for camera init
  eng(c).initCamera();     // was FUN_80050a80
  rc0(c, 0x80096a70);
  rc1(c, 0x80099310, 0x1010);
  rc1(c, 0x800991b0, 0x20000);
  rc1(c, 0x800993a0, 1);
  // The original CdControlB(Setmode) is intentionally absent: reads are native by LBA and there is
  // no emulated drive mode. Its following VSync settle waits are absent too; the host frame loop is
  // the sole timing owner and guest VSync is a trap.
  eng(c).font.init(); // was FUN_80075130
  rc1(c, 0x8009c620, 0);
  rc0(c, 0x8001cc00);
  eng(c).initSubsystems();           // was FUN_800520e0
  rc0(c, 0x80051e00);                // scheduler table: task objects at 0x801fe000
  rc2(c, 0x80051f14, 0, 0x800499e8); // register task 0 at FUN_800499e8
  // No preemptive VBlank IRQ is delivered. The native callback owner records that policy without
  // dereferencing the guest interrupt vector.
  c->game->timing.vsyncCallback();

  lucent::info("native_boot", "init prefix complete");

  // FUN_800499E8 resolves START.BIN and FUN_80052078(0) installs stage 0. The guest scheduler would
  // normally establish DAT_1f800138 in FUN_80051e60; task0Bootstrap needs it to name slot 0.
  c->mem_w32(0x1f800138, 0x801fe000);
  eng(c).task0Bootstrap(); // was FUN_800499e8; CD subtree is owned top-down
  lucent::info("native_boot",
               "after FUN_800499e8: START.BIN count@0x80106228={} "
               "entry-word@0x8010649c=0x{:08X} (expect 0x27BDFE38); task0 state={} "
               "entry=0x{:08X}",
               c->mem_r32(0x80106228),
               c->mem_r32(0x8010649c),
               c->mem_r16(0x801fe000),
               c->mem_r32(0x801fe00c));
}

} // namespace tomba
