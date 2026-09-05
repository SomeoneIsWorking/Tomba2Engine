#include "game.h"
#include "game_runtime.h"
#include "lightrec_executor.h"
#include "native_override_catalog.h"

#include <lucent/log.h>
#include <memory>

namespace {

class Runtime final : public GameRuntime {
public:
  void *createContext(Core &) override {
    return nullptr;
  }
  void destroyContext(void *) override {}
  void registerOverrides(Game &) override {}
  void bootInit(Core &) override {}
  RenderCapabilities renderCapabilities() const override {
    return RenderCapabilities::direct();
  }
  bool guestVramIsPicture(const Game &) const override {
    return false;
  }
};

constexpr std::uint32_t kEntry = 0x80010000u;
constexpr std::uint32_t kReturn = 0x80010100u;
constexpr GuestAddressRange kResidentText{0x10000u, 0x10110u};
int nativeCalls = 0;

void nativeOwner(Core *core) {
  ++nativeCalls;
  core->r[2] = 41u;
}

int failed = 0;
int checked = 0;

void check(bool condition, const char *name) {
  ++checked;
  if (!condition) {
    ++failed;
    lucent::error("native-catalog-test", "failed: {}", name);
  }
}

} // namespace

int main() {
  Runtime runtime;
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  core.mem_w32(kEntry, 0x24020005u);      // addiu v0, zero, 5
  core.mem_w32(kEntry + 4u, 0x03e00008u); // jr ra
  core.mem_w32(kEntry + 8u, 0u);
  const auto first = core.imageCatalog().activate("resident", kResidentText, 1u);
  tomba::native::declareOverride(kEntry, "native-owner", nativeOwner);
  tomba::native::bindResident(core, first, kResidentText);
  const auto budget = psx::cpu::ExecutionBudget::fromCycles(1000u);
  core.r[31] = kReturn;
  auto result = psx::cpu::dispatchGuest(core, kEntry, budget);
  check(result.returned() && core.r[2] == 41u && nativeCalls == 1, "first image selects native owner");

  const auto second = core.imageCatalog().activate("resident", kResidentText, 1u);
  check(second.generation != first.generation, "reloading advances the image generation");
  core.r[31] = kReturn;
  result = psx::cpu::dispatchGuest(core, kEntry, budget);
  check(result.returned() && core.r[2] == 5u && nativeCalls == 1,
        "stale image registration cannot select the native owner");

  tomba::native::bindResident(core, second, kResidentText);
  tomba::native::bindResident(core, second, kResidentText);
  core.r[31] = kReturn;
  result = psx::cpu::dispatchGuest(core, kEntry, budget);
  check(result.returned() && core.r[2] == 41u && nativeCalls == 2, "final image rebinding is effective and idempotent");

  core.r[31] = kReturn;
  result = psx::cpu::callOriginal(core, psx::cpu::NativeKey{second, kEntry}, budget);
  check(result.returned() && core.r[2] == 5u && nativeCalls == 2,
        "scoped original executes guest instructions without native recursion");

  core.mem_w32(kEntry, 0x24020009u); // different image: addiu v0, zero, 9
  const auto other = core.imageCatalog().activate("different-image", kResidentText, 2u);
  tomba::native::bindResident(core, second, kResidentText);
  core.r[31] = kReturn;
  result = psx::cpu::dispatchGuest(core, kEntry, budget);
  check(result.returned() && core.r[2] == 9u && nativeCalls == 2 &&
            !core.nativeDispatcher().isInstalled({other, kEntry}),
        "colliding image stays JIT and cannot acquire a resident native declaration");
  const auto &counters = core.lightrecExecutor().counters();
  check(counters.executedBlocks > 0 && counters.fallback.calls == 0,
        "original calls execute translated blocks without interpreter fallback");
  lucent::info("native-catalog-test", "checked={} failed={}", checked, failed);
  return failed == 0 ? 0 : 1;
}
