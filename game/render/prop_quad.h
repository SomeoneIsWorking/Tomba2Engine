// game/render/prop_quad.h — pure policy seam for FUN_8013CDD4's prop-quad display producer.
#pragma once

#include <cstdint>

class PropQuadRecipe {
public:
  // The guest multiplies in an 8-bit stack slot before ObjModelView widens the result and shifts it.
  // Preserve that wrap exactly; doing the multiply in int32 would produce a different large scale.
  static constexpr int32_t columnScale(uint8_t authored) {
    return (int32_t)(uint8_t)(authored * 10u) << 2;
  }

  // -1 means keep the packed record's tpage. Otherwise FUN_8013CDD4 forces tpage 46.
  static constexpr int tpageOverride(uint8_t renderKind, uint8_t renderSubKind) {
    if (renderKind == 1u || renderKind == 3u) {
      return -1;
    }
    return ((renderSubKind >= 3u && renderSubKind < 6u) || (renderSubKind >= 9u && renderSubKind < 11u)) ? 46 : -1;
  }
};
