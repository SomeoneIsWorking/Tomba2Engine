#pragma once

#include "game_iface.h"

namespace tomba {

// Process-lifetime owner of Tomba! 2's framework-facing behavior. The legacy base is temporary:
// measured compatibility facts and callbacks remain reachable while psxport replaces their generic
// consumers with narrow typed interfaces.
class TombaRuntime final : public LegacyGameRuntimeAdapter {
public:
  TombaRuntime();

  void *createContext(Core &core) override;
  void destroyContext(void *context) override;
  void registerOverrides(Game &game) override;
  void bootInit(Core &core) override;
  bool guestVramIsPicture(const Game &game) const override;
};

} // namespace tomba
