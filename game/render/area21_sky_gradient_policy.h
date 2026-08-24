#pragma once

#include <array>
#include <cstdint>

namespace Area21SkyGradientPolicy {

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct Band {
  int top;
  int bottom;
  Color topColor;
  Color bottomColor;
};

// FUN_8003DF04 selects the A0L composite drawer only for background state 21. Inside
// FUN_8010BE30, variant 1 and phase < 4 call FUN_8010BB64 and return immediately; the tilemap loop is
// a different branch. Keep this predicate as the single owner of that reached control-flow decision.
constexpr bool active(uint8_t dispatchGate, uint8_t backgroundState, uint8_t variant, uint8_t phase) {
  return dispatchGate == 0u && backgroundState == 21u && variant == 1u && phase < 4u;
}

// FUN_8010BB64 expands this exact signed expression before the arithmetic shift.
constexpr int originY(int16_t pitch) {
  return (-(int32_t)pitch * 2280 >> 12) + 120;
}

constexpr std::array<Band, 4> bands(int16_t pitch) {
  constexpr Color kUpper{6u, 6u, 172u};       // guest word 0x00AC0606
  constexpr Color kHorizon{152u, 152u, 234u}; // guest word 0x00EA9898
  constexpr Color kLower{0u, 0u, 57u};        // guest word 0x00390000
  const int y = originY(pitch);
  return {{{y - 440, y - 60, kUpper, kUpper},
           {y - 60, y, kUpper, kHorizon},
           {y, y + 60, kHorizon, kLower},
           {y + 60, y + 480, kLower, kLower}}};
}

} // namespace Area21SkyGradientPolicy
