#pragma once

struct GameConfig;
struct GameHooks;

namespace tomba::legacy {

// Compatibility debt for generic psxport algorithms that still read Core::cfg/Core::hooks.
// TombaRuntime is the title's ownership seam; new behavior and facts do not belong here.
extern const GameConfig &measuredConfig;
extern const GameHooks &compatibilityHooks;

} // namespace tomba::legacy
