#pragma once

#include "auto_drive.h"
#include "frame_diagnostics.h"
#include "game_runtime.h"

class Game;

namespace tomba {

// Tomba! 2's one finite logic-frame transaction. The framework owns only repetition around this
// object; every title-shaped input, timing, scheduler, render, and presentation operation lives here.
class TombaFrameDriver final : public FrameDriver {
public:
  explicit TombaFrameDriver(Game &game);

  void stepFrame(Core &core, uint32_t frame) override;

private:
  Game *game_;
  AutoDrive autoDrive_;
  FrameDiagnostics diagnostics_;
};

} // namespace tomba
