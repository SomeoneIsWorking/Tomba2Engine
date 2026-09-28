#pragma once

#include "execution_exit.h"
#include "game_runtime.h"
#include "r3000.h"

#include <array>
#include <cstddef>
#include <cstdint>

class Game;
class Core;

namespace tomba1 {

class Tomba1Runtime;

class Tomba1FrameDriver final : public FrameDriver {
public:
  Tomba1FrameDriver(Game &game, Tomba1Runtime &runtime);

  void stepFrame(Core &core, std::uint32_t frame) override;

  static void startOverride(Core *core);
  static void yieldOverride(Core *core);
  static void restartOverride(Core *core);

private:
  struct TaskSlot {
    R3000 context{};
    std::uint32_t entry = 0;
    std::uint32_t resumeAddress = 0;
    // How many display fields this slot has been resumed across WITHOUT reaching its cooperative
    // yield. Reset by every yield and by every fresh start, so it measures one unbroken guest call,
    // not the slot's lifetime. See `kMaxBudgetResumesPerCall` for what bounds it.
    std::uint32_t budgetResumes = 0;
    bool contextReady = false;
  };

  void runScheduledTasks(Core &core);
  void runTaskSlot(Core &core, std::size_t slot, std::uint16_t state, const R3000 &loopContext);

  // Takes the EXITED task's saved context explicitly, so the report cannot be called with the frame
  // loop's own registers by mistake. `runTaskSlot` already copied them out of the Core before
  // restoring the loop context, and it owns the copy; handing the context over keeps this a
  // formatter and leaves the one place that has the state responsible for getting it right.
  static void
  reportGuestExit(Core &core, std::size_t slot, const psx::cpu::ExecutionResult &result, const R3000 &exited);
  void registerTaskStart(Core &core, std::size_t slot, std::uint32_t entry);
  void yieldTask(Core &core);
  void restartTask(Core &core);
  void finishMainIteration(Core &core, std::uint32_t fields);
  // The one budget exit path: records the resume, re-arms the slot for the next field, and either
  // returns (progress) or reports and stops (no progress). Kept beside `runTaskSlot` rather than
  // inline because the two decisions it makes — is this the same call, and has it gone on too long —
  // are the whole of the policy, and inlining them buries them in the dispatch sequence.
  bool resumeAcrossField(Core &core, std::size_t slot, const psx::cpu::ExecutionResult &result);

  Game &game_;
  Tomba1Runtime &runtime_;
  std::array<TaskSlot, 3> tasks_{};
  std::uint32_t completedFrames_ = 0;
  bool booted_ = false;
  bool restartRequested_ = false;
  std::size_t activeSlot_ = tasks_.size();

  static Tomba1FrameDriver *active_;
};

} // namespace tomba1
