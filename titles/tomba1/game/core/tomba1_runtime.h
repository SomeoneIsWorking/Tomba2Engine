#pragma once

#include "execution_exit.h"
#include "game_runtime.h"

#include <cstdint>
#include <memory>

namespace tomba1 {

class Tomba1Runtime final : public GameRuntime {
public:
  Tomba1Runtime() = default;

  void *createContext(Core &core) override;
  void destroyContext(void *context) override;
  void registerOverrides(Game &game) override;
  void bootInit(Core &core) override;
  std::unique_ptr<FrameDriver> createFrameDriver(Game &game) override;
  const GuestProgramImage *guestProgramImage() const override;
  const PlatformHlePlan *platformHlePlan() const override;
  const GuestPadBufferLayout *guestPadBufferLayout() const override;
  const GuestCdStreamCallbackLayout *guestCdStreamCallbackLayout() const override;
  RenderCapabilities renderCapabilities() const override;
  bool guestVramIsPicture(const Game &game) const override;

  psx::cpu::ExecutionResult dispatchUntilExit(Core &core, std::uint32_t address) const;

private:
  static const GuestProgramImage programImage_;
  static const GuestPadBufferLayout padBufferLayout_;
};

} // namespace tomba1
