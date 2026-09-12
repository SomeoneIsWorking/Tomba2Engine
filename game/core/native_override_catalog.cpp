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
  std::string imageName;
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
    return entry.imageName.empty() && entry.address == address;
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
  entries.push_back({{}, address, std::string(name), function});
}

void declareOverlayOverride(std::string_view imageName,
                            std::uint32_t address,
                            std::string_view name,
                            psx::cpu::NativeFunction function) {
  if (imageName.empty() || address == 0u || name.empty() || function == nullptr) {
    lucent::error("tomba-native", "refused incomplete overlay override declaration at 0x{:08X}", address);
    std::abort();
  }

  auto &entries = declarations();
  const auto existing = std::find_if(entries.begin(), entries.end(), [&](const Declaration &entry) {
    return entry.imageName == imageName && entry.address == address;
  });
  if (existing != entries.end()) {
    if (existing->function == function && existing->name == name) {
      return;
    }
    lucent::error("tomba-native",
                  "refused conflicting '{}' overlay overrides '{}' and '{}' at 0x{:08X}",
                  imageName,
                  existing->name,
                  name,
                  address);
    std::abort();
  }
  entries.push_back({std::string(imageName), address, std::string(name), function});
}

void bindResident(Core &core, psx::cpu::ImageIdentity resident, GuestAddressRange residentText) {
  std::size_t installed = 0;
  std::size_t retained = 0;
  std::size_t inactive = 0;
  for (const Declaration &entry : declarations()) {
    const auto identity = core.currentImageIdentity(entry.address);
    if (!entry.imageName.empty() || !residentText.containsPhysical(entry.address) || !identity ||
        *identity != resident) {
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

void bindOverlay(Core &core,
                 psx::cpu::ImageIdentity overlay,
                 std::string_view imageName,
                 GuestAddressRange overlayText) {
  std::size_t installed = 0;
  std::size_t retained = 0;
  std::size_t inactive = 0;
  for (const Declaration &entry : declarations()) {
    const auto identity = core.currentImageIdentity(entry.address);
    if (entry.imageName != imageName || !overlayText.containsPhysical(entry.address) || !identity ||
        *identity != overlay) {
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
                    "failed to bind '{}' to active overlay {}:{} at 0x{:08X}",
                    entry.name,
                    identity->id,
                    identity->generation,
                    entry.address);
      std::abort();
    }
    ++installed;
  }
  lucent::info("tomba-native",
               "bound overlay '{}' generation: declarations={} installed={} retained={} inactive={}",
               imageName,
               declarations().size(),
               installed,
               retained,
               inactive);
}

psx::cpu::ImageIdentity activateOverlay(Core &core,
                                        std::optional<psx::cpu::ImageIdentity> &active,
                                        std::string_view imageName,
                                        GuestAddressRange text) {
  if (imageName.empty() || !text.valid() || text.end > 0x00200000u) {
    lucent::error(
        "tomba-native", "refused invalid overlay image '{}' range [{:08X}, {:08X})", imageName, text.begin, text.end);
    std::abort();
  }
  if (active) {
    for (const Declaration &entry : declarations()) {
      core.nativeDispatcher().remove({*active, entry.address});
    }
    if (!core.imageCatalog().deactivate(*active)) {
      lucent::error(
          "tomba-native", "active overlay image {}:{} disappeared before replacement", active->id, active->generation);
      std::abort();
    }
  }

  std::uint64_t contentIdentity = 1469598103934665603ull;
  for (std::uint32_t address = text.begin; address < text.end; ++address) {
    contentIdentity ^= core.mem_r8(address);
    contentIdentity *= 1099511628211ull;
  }
  const auto identity = core.imageCatalog().activate(imageName, text, contentIdentity);
  bindOverlay(core, identity, imageName, text);
  active = identity;
  return identity;
}

std::string overlayNameForArea(std::uint32_t area) {
  if (area >= 22u) {
    lucent::error("tomba-native", "area {} is outside the A00..A0L overlay set", area);
    std::abort();
  }
  const char suffix = area < 10u ? static_cast<char>('0' + area) : static_cast<char>('A' + area - 10u);
  std::string name = "A0";
  name.push_back(suffix);
  return name;
}

psx::cpu::ImageIdentity activateAreaOverlay(Core &core,
                                            std::optional<psx::cpu::ImageIdentity> &active,
                                            std::uint32_t area,
                                            std::uint32_t size) {
  constexpr std::uint32_t kModeSlot = 0x00108F9Cu;
  if (size == 0u || size > 0x00200000u - kModeSlot) {
    lucent::error("tomba-native", "refused invalid area {} overlay size {}", area, size);
    std::abort();
  }
  return activateOverlay(core, active, overlayNameForArea(area), {kModeSlot, kModeSlot + size});
}

} // namespace tomba::native
