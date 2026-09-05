#include "frame_driver.h"

#include "core.h"
#include "execution_control.h"
#include "game.h"
#include "guest_call.h"
#include "stream_field_turn.h"
#include "tomba1_runtime.h"

#include <cstdlib>
#include <lucent/log.h>

namespace tomba1 {
namespace {

constexpr std::uint32_t kTaskTableBegin = 0x801FD800u;
constexpr std::uint32_t kTaskTableEnd = 0x801FD950u;
constexpr std::uint32_t kTaskStride = 0x70u;
constexpr std::uint32_t kCurrentTask = 0x1F8001D4u;

constexpr std::uint32_t kVblankEventClass = 0xF2000003u;
constexpr std::uint32_t kVblankEventSpec = 2u;
constexpr std::uint32_t kRequestedFields = 0x1F8001EAu;
constexpr std::uint32_t kDrawSyncBeforeVblank = 0x1F8001ECu;
constexpr std::uint32_t kDrawSync = 0x8005EB54u;
constexpr std::uint32_t kResetGraph = 0x8005E694u;
constexpr std::uint32_t kDisplaySwap = 0x80016940u;
constexpr std::uint32_t kTickTaskSleeps = 0x800173B0u;

bool validTaskRecord(std::uint32_t record) {
  return record >= kTaskTableBegin && record < kTaskTableEnd && (record - kTaskTableBegin) % kTaskStride == 0u;
}

} // namespace

Tomba1FrameDriver *Tomba1FrameDriver::active_ = nullptr;

Tomba1FrameDriver::Tomba1FrameDriver(Game &game, Tomba1Runtime &runtime) : game_(game), runtime_(runtime) {}

void Tomba1FrameDriver::registerTaskStart(Core &core, std::size_t slot, std::uint32_t entry) {
  if (slot >= tasks_.size() || entry == 0u) {
    lucent::error("tomba1-frame", "invalid task start: slot={} entry=0x{:08X}", slot, entry);
    std::abort();
  }
  if (slot == activeSlot_) {
    lucent::error("tomba1-frame", "guest tried to replace its own live task slot {}", slot);
    std::abort();
  }
  const std::uint32_t record = kTaskTableBegin + static_cast<std::uint32_t>(slot) * kTaskStride;
  TaskSlot &task = tasks_[slot];
  task.entry = entry;
  task.resumeAddress = entry;
  task.contextReady = false;
  core.mem_w16(record, 2u);
}

void Tomba1FrameDriver::runTaskSlot(Core &core, std::size_t slot, std::uint16_t state, const R3000 &loopContext) {
  const std::uint32_t record = kTaskTableBegin + static_cast<std::uint32_t>(slot) * kTaskStride;
  TaskSlot &task = tasks_[slot];
  for (int restartBudget = 0; restartBudget < 4; ++restartBudget) {
    if (state == 3u || !task.contextReady) {
      if (state == 3u) {
        task.entry = core.mem_r32(record + 12u);
      }
      const std::uint32_t entry = task.entry;
      if (entry == 0u) {
        lucent::error("tomba1-frame", "runnable task 0x{:08X} was not started by the native task owner", record);
        std::abort();
      }
      task.context = loopContext;
      task.context.r[29] = core.mem_r32(record + 8u);
      task.context.r[31] = 0xDEAD0000u;
      task.context.pc = entry;
      task.resumeAddress = entry;
      task.contextReady = true;
    }
    if (!task.contextReady) {
      lucent::error("tomba1-frame", "task {} has no saved CPU register context", slot);
      std::abort();
    }

    core.mem_w32(kCurrentTask, record);
    core.mem_w16(record, 4u);
    restartRequested_ = false;
    activeSlot_ = slot;
    active_ = this;
    static_cast<R3000 &>(core) = task.context;
    const psx::cpu::ExecutionResult result = runtime_.dispatchUntilExit(core, task.resumeAddress);
    task.context = static_cast<R3000 &>(core);
    static_cast<R3000 &>(core) = loopContext;
    active_ = nullptr;
    activeSlot_ = tasks_.size();

    if (restartRequested_) {
      state = 3u;
      continue;
    }
    if (result.reason == psx::cpu::ExecutionExitReason::CooperativeYield) {
      task.resumeAddress = result.guestPc;
      return;
    }
    if (result.returned()) {
      lucent::error("tomba1-frame", "guest task {} returned instead of reaching its cooperative yield", slot);
      std::abort();
    }
    lucent::error("tomba1-frame",
                  "guest task {} exited with {} at 0x{:08X}: {}",
                  slot,
                  psx::cpu::executionExitName(result.reason),
                  result.guestPc,
                  result.detail);
    std::abort();
  }

  lucent::error("tomba1-frame", "task restarted more than four times in one native frame");
  std::abort();
}

void Tomba1FrameDriver::startOverride(Core *core) {
  if (!active_ || !core) {
    lucent::error("tomba1-frame", "guest task start reached outside the native frame owner");
    std::abort();
  }
  active_->registerTaskStart(*core, core->r[4], core->r[5]);
}

void Tomba1FrameDriver::runScheduledTasks(Core &core) {
  const R3000 loopContext = static_cast<R3000 &>(core);
  for (std::size_t slot = 0; slot < tasks_.size(); ++slot) {
    const std::uint32_t record = kTaskTableBegin + static_cast<std::uint32_t>(slot) * kTaskStride;
    const std::uint16_t state = core.mem_r16(record);
    if (state == 2u || state == 3u) {
      runTaskSlot(core, slot, state, loopContext);
    }
  }
}

void Tomba1FrameDriver::yieldTask(Core &core) {
  const std::uint32_t record = core.mem_r32(kCurrentTask);
  if (!validTaskRecord(record) || activeSlot_ >= tasks_.size()) {
    lucent::error("tomba1-frame", "cooperative yield has no active measured task");
    std::abort();
  }
  TaskSlot &task = tasks_[activeSlot_];
  if (!task.contextReady || record != kTaskTableBegin + static_cast<std::uint32_t>(activeSlot_) * kTaskStride) {
    lucent::error("tomba1-frame", "cooperative yield changed the active task record");
    std::abort();
  }
  core.mem_w16(record + 2u, static_cast<std::uint16_t>(core.r[4]));
  core.mem_w16(record, 1u);
  task.context = static_cast<R3000 &>(core);
  task.contextReady = true;
  psx::cpu::requestExecutionExit(core, psx::cpu::ExecutionExitReason::CooperativeYield);
}

void Tomba1FrameDriver::restartTask(Core &core) {
  const std::uint32_t record = core.mem_r32(kCurrentTask);
  if (!validTaskRecord(record) || activeSlot_ >= tasks_.size()) {
    lucent::error("tomba1-frame", "task restart has no active measured task");
    std::abort();
  }
  TaskSlot &task = tasks_[activeSlot_];
  if (!task.contextReady || record != kTaskTableBegin + static_cast<std::uint32_t>(activeSlot_) * kTaskStride) {
    lucent::error("tomba1-frame", "task restart changed the active task record");
    std::abort();
  }
  core.mem_w16(record, 3u);
  core.mem_w32(record + 12u, core.r[4]);
  restartRequested_ = true;
  psx::cpu::requestExecutionExit(core, psx::cpu::ExecutionExitReason::CooperativeYield);
}

void Tomba1FrameDriver::yieldOverride(Core *core) {
  if (!active_ || !core) {
    lucent::error("tomba1-frame", "guest yield reached outside the native frame owner");
    std::abort();
  }
  active_->yieldTask(*core);
}

void Tomba1FrameDriver::restartOverride(Core *core) {
  if (!active_ || !core) {
    lucent::error("tomba1-frame", "guest task restart reached outside the native frame owner");
    std::abort();
  }
  active_->restartTask(*core);
}

void Tomba1FrameDriver::finishMainIteration(Core &core, std::uint32_t fields) {
  if (core.mem_r16(kDrawSyncBeforeVblank) != 0u) {
    psx::cpu::dispatchGuestToReturn1(core, kDrawSync, 0u, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
  }

  for (std::uint32_t field = 0; field < fields; ++field) {
    game_.hle.deliverEvent(kVblankEventClass, kVblankEventSpec);
    game_.spu_audio.frame();
    noteNativeFieldDelivered(core);
  }

  if (core.mem_r16(kDrawSyncBeforeVblank) == 0u) {
    psx::cpu::dispatchGuestToReturn1(core, kResetGraph, 1u, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
  }

  const std::uint8_t displayState = core.mem_r8(0x1F8001CCu);
  if ((displayState < 2u || displayState == 3u) && core.mem_r16(0x1F8001F0u) < 0x4001u) {
    if (displayState == 3u) {
      core.mem_w8(0x1F8001CCu, 2u);
    }
    psx::cpu::dispatchGuestToReturn0(core, kDisplaySwap, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
    psx::cpu::dispatchGuestToReturn0(core, kTickTaskSleeps, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
  }
}

void Tomba1FrameDriver::stepFrame(Core &core, std::uint32_t frame) {
  if (core.game != &game_ || frame != completedFrames_) {
    lucent::error("tomba1-frame",
                  "invalid frame request: game-bound={} requested={} completed={}",
                  core.game == &game_,
                  frame,
                  completedFrames_);
    std::abort();
  }

  game_.timing.logicFrame = frame;
  game_.timing.frameTick();
  core.rsub.otAttr.beginLogicFrame(frame);
  game_.pad.serviceFrame();

  if (!booted_) {
    active_ = this;
    runtime_.bootInit(core);
    active_ = nullptr;
    booted_ = true;
  }

  core.mem_w16(0x1F8001E8u, 0u);
  if (core.mem_r16(0x1F8001F0u) < 0x4001u) {
    const std::int16_t parity = static_cast<std::int16_t>(core.mem_r16(0x1F8001F4u));
    core.mem_w32(0x8009C8A8u, static_cast<std::uint32_t>(parity * 0x780 + 0x800A1890));
  }

  runScheduledTasks(core);

  const std::uint32_t fields = core.mem_r16(kRequestedFields);
  if (fields == 0u || fields > 4u) {
    lucent::error("tomba1-frame", "retail frame requested {} display fields; expected 1..4", fields);
    std::abort();
  }
  finishMainIteration(core, fields);
  game_.presentation.commit(&core, static_cast<int>(fields), game_.temporalPresentation.get());
  ++completedFrames_;
}

} // namespace tomba1
