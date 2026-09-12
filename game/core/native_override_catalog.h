#pragma once

#include "native_dispatch.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

class Core;

namespace tomba::native {

// Records a title-owned native implementation without binding it to an address-only global table.
// Declarations are process-lifetime metadata; binding requires an explicit resident-image token.
void declareOverride(std::uint32_t address, std::string_view name, psx::cpu::NativeFunction function);

// Records an implementation that belongs to one named runtime overlay. Overlay declarations may
// share a numeric address with resident or other-overlay declarations; the image name is part of
// the ownership key and is checked again when the image is bound.
void declareOverlayOverride(std::string_view imageName,
                            std::uint32_t address,
                            std::string_view name,
                            psx::cpu::NativeFunction function);

// Installs resident declarations only when the supplied image still owns the address. The caller
// obtains the token at resident load, never by adopting an arbitrary active overlay. Repeated calls
// are idempotent. This is a residency contract, not executable-content authentication.
void bindResident(Core &core, psx::cpu::ImageIdentity resident, GuestAddressRange residentText);

// Activates the bytes already loaded in `text`, retires the previous image in the same MODE slot,
// and binds declarations whose overlay name and address range match. The content identity is
// derived from the loaded bytes here so callers cannot accidentally publish a stale or fabricated
// token. `active` is per-Core state owned by the MODE loader.
psx::cpu::ImageIdentity activateOverlay(Core &core,
                                        std::optional<psx::cpu::ImageIdentity> &active,
                                        std::string_view imageName,
                                        GuestAddressRange text);

// Publishes the field-code image loaded in the fixed MODE slot. The size comes from the same
// authenticated GAME.BIN descriptor table used by FUN_80045080, so the active range covers exactly
// the bytes read by the loader.
psx::cpu::ImageIdentity
activateAreaOverlay(Core &core, std::optional<psx::cpu::ImageIdentity> &active, std::uint32_t area, std::uint32_t size);

// The disc's 22 field-code files are named A00..A0L in area-index order.
std::string overlayNameForArea(std::uint32_t area);

} // namespace tomba::native
