#pragma once

#include "native_dispatch.h"

#include <cstdint>
#include <string_view>

class Core;

namespace tomba::native {

// Records a title-owned native implementation without binding it to an address-only global table.
// Declarations are process-lifetime metadata; binding requires an explicit resident-image token.
void declareOverride(std::uint32_t address, std::string_view name, psx::cpu::NativeFunction function);

// Installs resident declarations only when the supplied image still owns the address. The caller
// obtains the token at resident load, never by adopting an arbitrary active overlay. Repeated calls
// are idempotent. This is a residency contract, not executable-content authentication.
void bindResident(Core &core, psx::cpu::ImageIdentity resident, GuestAddressRange residentText);

} // namespace tomba::native
