#include "native_override_catalog.h"

#include "core.h"

#include <algorithm>
#include <cstdlib>
#include <lucent/log.h>
#include <string>
#include <vector>

namespace tomba::native {
namespace {

struct Declaration {
  std::uint32_t address = 0;
  std::string name;
  psx::cpu::NativeFunction function = nullptr;
};

std::vector<Declaration> &declarations() {
  static std::vector<Declaration> value;
  return value;
}

} // namespace

void declareOverride(std::uint32_t address, std::string_view name, psx::cpu::NativeFunction function) {
  if (address == 0u || name.empty() || function == nullptr) {
    lucent::error("tomba-native", "refused incomplete native override declaration at 0x{:08X}", address);
    std::abort();
  }

  auto &entries = declarations();
  const auto existing = std::find_if(entries.begin(), entries.end(), [address](const Declaration &entry) {
    return entry.address == address;
  });
  if (existing != entries.end()) {
    if (existing->function == function && existing->name == name) {
      return;
    }
    lucent::error("tomba-native",
                  "refused conflicting native overrides '{}' and '{}' at 0x{:08X}",
                  existing->name,
                  name,
                  address);
    std::abort();
  }
  entries.push_back({address, std::string(name), function});
}

void bindResident(Core &core, psx::cpu::ImageIdentity resident, GuestAddressRange residentText) {
  std::size_t installed = 0;
  std::size_t retained = 0;
  std::size_t inactive = 0;
  for (const Declaration &entry : declarations()) {
    const auto identity = core.currentImageIdentity(entry.address);
    if (!residentText.containsPhysical(entry.address) || !identity || *identity != resident) {
      ++inactive;
      continue;
    }
    const psx::cpu::NativeKey key{*identity, entry.address};
    if (core.nativeDispatcher().isInstalled(key)) {
      ++retained;
      continue;
    }
    if (!core.nativeDispatcher().install({key, entry.name, entry.function})) {
      lucent::error("tomba-native",
                    "failed to bind '{}' to active image {}:{} at 0x{:08X}",
                    entry.name,
                    identity->id,
                    identity->generation,
                    entry.address);
      std::abort();
    }
    ++installed;
  }
  lucent::info("tomba-native",
               "bound resident image generation: declarations={} installed={} retained={} inactive={}",
               declarations().size(),
               installed,
               retained,
               inactive);
}

} // namespace tomba::native
