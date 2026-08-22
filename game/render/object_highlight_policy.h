#pragma once

#include <cstdint>

namespace ObjectHighlightPolicy {

struct Activation {
  bool enabled;
  int32_t scaleInput;
};

struct Cue {
  int32_t amount;
  int32_t sortBias;
};

// The queue-A walk uses the node TYPE byte as both its jump-table index and the highlight selector.
// Bit 0x40 selects the fixed 0x50 scale; bit 0x80 selects the node's signed scale field. With neither
// bit set the guest does not call FUN_8002AE0C at all.
constexpr Activation activation(uint8_t type, int16_t dynamicScale) {
  if ((type & 0x40u) != 0u) {
    return Activation{true, 0x50};
  }
  if ((type & 0x80u) != 0u) {
    return Activation{true, dynamicScale};
  }
  return Activation{false, 0};
}

// FUN_8002AE0C selects both depth cue and OT bias from the field mode byte at 0x800BF870.
constexpr Cue cue(uint8_t fieldMode) {
  if (fieldMode == 1u || fieldMode == 2u) {
    return Cue{2048, -10};
  }
  if (fieldMode == 7u) {
    return Cue{1024, -3};
  }
  return Cue{0, -10};
}

// The guest arithmetic-shifts the signed input, stores the result to an unsigned byte, then widens
// that byte and multiplies by four. Preserve that byte wrap; it is observable for negative inputs.
constexpr int32_t lateralScale(int32_t scaleInput) {
  return (int32_t)(uint8_t)(scaleInput >> 3) * 4;
}

} // namespace ObjectHighlightPolicy
