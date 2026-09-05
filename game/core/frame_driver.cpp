#include "frame_driver.h"

#include "cfg.h"
#include "core.h"
#include "dev_warp.h"
#include "game.h"
#include "game_ctx.h"
#include "gpu_vk.h"
#include "guest_call.h"
#include "hw_bind.h"
#include "render.h"

#include <cstdlib>
#include <lucent/log.h>

void gpu_set_disp_origin(Core *core, int x, int y);

namespace tomba {
namespace {

const GameConfig &requireMeasuredConfig(Core &core) {
  if (!core.cfg) {
    lucent::error("tomba-frame", "TombaFrameDriver requires Tomba! 2's measured compatibility facts");
    std::abort();
  }
  return *core.cfg;
}

void applyArmedStandaloneWarp(Core &core, uint32_t frame) {
  Game &game = *core.game;
  if (!game.repl.warpArmed) {
    return;
  }

  game.repl.warpArmed = 0;
  const uint32_t dest = game.repl.warpDest & 0x1fu;
  applyColdWarp(core, static_cast<int>(dest), static_cast<int>(game.repl.warpSub & 0x3fu));
  lucent::info("repl", "warp: cold area {} sub {} loaded at f{}", dest, game.repl.warpSub, frame);
}

void bindFrameHardware(Core &core) {
  gte_bind(&core);
  if (gpu_vk_wide_engine(&core)) {
    const int ofx = gpu_vk_wide_engine_ofx(&core);
    gte_write_ctrl(24u, static_cast<uint32_t>(ofx) << 16);
    core.rsub.projParams.setGeomOfxForAspect(static_cast<float>(ofx));
  }
  core.rsub.projprim.bind(&core);
  spu_bind(&core);
  mdec_bind(&core);
  xa_bind(&core);
}

void resetSingleBufferTail(Core &core, const GameConfig &cfg) {
  core.mem_w16(cfg.dwellCounter, 0);
  core.mem_w32(cfg.poolPtrLast, core.mem_r32(cfg.poolPtrCur));
  core.mem_w32(cfg.poolPtrCur, cfg.packetPoolBase & 0xffffffu);
}

void preparePrimarySingleBuffer(Core &core, const GameConfig &cfg, uint32_t envp) {
  // Retail main-loop order: publish the primary OT base before clearing that OT.
  core.mem_w32(cfg.otBasePtr, envp);
  psx::cpu::dispatchGuestToReturn2(
      core, cfg.clearOtagR, envp, 0x800, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
  resetSingleBufferTail(core, cfg);
}

void submitFrame(Core &core, const GameConfig &cfg, uint32_t envp) {
  if (core.mem_r16(0x1f80019cu) != 0) {
    return;
  }

  psx::cpu::dispatchGuestToReturn1(
      core, cfg.putDrawEnv, envp + 0x2014u, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
  gpu_set_disp_origin(&core, 0, 0);
  eng(&core).drawOTag(envp + 0x1ffcu);
}

} // namespace

TombaFrameDriver::TombaFrameDriver(Game &game) : game_(&game) {}

void TombaFrameDriver::stepFrame(Core &core, uint32_t frame) {
  if (!core.game || core.game != game_) {
    lucent::error("tomba-frame", "TombaFrameDriver used with a different or unbound Game");
    std::abort();
  }

  Game &game = *game_;
  const GameConfig &cfg = requireMeasuredConfig(core);
  const uint32_t envp = cfg.otRegionBase; // PC-native presentation pins Tomba! 2 to buffer parity zero.

  core.rsub.otAttr.beginLogicFrame(frame);
  autoDrive_.beforeFrame(core, frame);
  bindFrameHardware(core);
  game.timing.logicFrame = frame;
  game.perf.frameBegin();
  game.timing.frameTick();
  for (uint32_t eventClass : cfg.irqEventClasses) {
    game.hle.deliverEvent(eventClass, 0xffffffffu);
  }

  preparePrimarySingleBuffer(core, cfg, envp);
  game.pad.serviceFrame();
  game.cd.audioTrace("pre");
  game.perf.markPre();

  // The pending capture is consumed before any producer begins this frame. Engine::frameUpdate owns
  // Tomba's pad-edge and per-VBlank audio work; this driver owns the one presentation fence.
  eng(&core).frameUpdate();
  game.perf.phaseBegin(2);
  game.presentation.commit(&core, 0, game.temporalPresentation.get());
  game.perf.phaseEnd(2);

  rend(&core)->bbFrameReset();
  applyArmedStandaloneWarp(core, frame);
  game.cd.audioTrace("post");
  game.perf.phaseBegin(3);
  game.pcSched.step();
  game.perf.phaseEnd(3);

  eng(&core).musicCoord.tick();
  game.cd.audioTrace("coord");
  psx::cpu::dispatchGuestToReturn1(core, cfg.drawSync, 0, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
  game.pcSched.tickSleepCountdown();
  submitFrame(core, cfg, envp);
  game.perf.frameEnd();
  diagnostics_.afterFrame(core, frame);
  autoDrive_.afterFrame(core, frame);
}

} // namespace tomba
