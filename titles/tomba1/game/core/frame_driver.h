#pragma once

#include "game_runtime.h"
#include "r3000.h"

#include <array>
#include <cstddef>
#include <cstdint>

class Game;

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
    bool contextReady = false;
  };

  void runScheduledTasks(Core &core);
  void runTaskSlot(Core &core, std::size_t slot, std::uint16_t state, const R3000 &loopContext);
  void registerTaskStart(Core &core, std::size_t slot, std::uint32_t entry);
  void yieldTask(Core &core);
  void restartTask(Core &core);
  void finishMainIteration(Core &core, std::uint32_t fields);

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
