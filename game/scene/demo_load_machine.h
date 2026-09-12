#pragma once

#include <cstdint>

class Core;

namespace tomba::demo {

inline constexpr std::uint32_t kStatePtr = 0x1f800138u;

// Advance DEMO s4's load-menu sub-machine, including CRD image residency.
void stepLoadMachine(Core &core);

} // namespace tomba::demo
