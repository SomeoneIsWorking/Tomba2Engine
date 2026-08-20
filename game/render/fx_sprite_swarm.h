// game/render/fx_sprite_swarm.h — FUN_800281EC, the PER-PARTICLE (swarm) member of the
// FUN_80027A4C world-anchored scaled-sprite family.
//
// See fx_sprite_swarm.cpp for the identification evidence and for what separates this emitter from
// its two siblings. This header holds only the typed LENSES over the guest blocks the function
// reads and writes, plus a named constant for every field offset, so the body reads as "stamp the
// node's sprite once per particle" rather than as a run of mem_r32(base + 0x4E). The five-word
// scratchpad handoff every member of the family shares lives in fx_sprite_publish.h.
#pragma once
#include "core.h"
#include "fx_sprite_publish.h"
#include <stdint.h>

class Game;

// ------------------------------------------------------------------------------------------------
// The type-0x20 RENDER NODE this family hangs off. The field object walk dispatches the node's own
// render fn at +0x18; for this member that fn is 0x800281EC. Unlike its two siblings the node's own
// world anchor at +0x2C/+0x30 is NEVER read — the anchors live in the particle array at +0x50.
namespace fxswarm {
constexpr uint32_t kSizeClass = 0x03;     // u8  — node "kind" tag; '!' picks the small size class
constexpr uint32_t kOtBias = 0x32;        // s16 — bias added to the OT bucket key
constexpr uint32_t kRecordHead = 0x34;    // u32 — head of the 8-byte sprite-record list (writer a0)
constexpr uint32_t kClut = 0x44;          // u16 — texture CLUT id     } together the writer's a1
constexpr uint32_t kTexturePage = 0x46;   // u16 — texture page bits   }
constexpr uint32_t kParticleCount = 0x4E; // s16 — live particles in the array below
constexpr uint32_t kParticleArray = 0x50; // the particle array itself, stride 8

// One PARTICLE. The size multiplier is packed into the HIGH half of the Z word — the emitter feeds
// the whole 32-bit word to the GTE's VZ0 (which only consumes the low 16 bits) and reads the high
// half back separately as the per-particle size.
constexpr uint32_t kParticleStride = 8;
constexpr uint32_t kPartPosXY = 0x00;   // u32 — packed world VX (lo16) | VY (hi16)
constexpr uint32_t kPartPosZ = 0x04;    // u32 — world VZ in the low 16 bits
constexpr uint32_t kPartSizeMul = 0x06; // s16 — this particle's own size, 8-bit fraction
} // namespace fxswarm

// Read-only lens over the render node.
struct FxSwarmNode {
  Core *mCore;
  uint32_t mBase;

  uint32_t sizeClassTag() const {
    return mCore->mem_r8(mBase + fxswarm::kSizeClass);
  }
  int32_t otBias() const {
    return mCore->mem_r16s(mBase + fxswarm::kOtBias);
  }
  uint32_t recordHead() const {
    return mCore->mem_r32(mBase + fxswarm::kRecordHead);
  }
  uint32_t clut() const {
    return mCore->mem_r16(mBase + fxswarm::kClut);
  }
  uint32_t texturePage() const {
    return mCore->mem_r16(mBase + fxswarm::kTexturePage);
  }
  int32_t particleCount() const {
    return mCore->mem_r16s(mBase + fxswarm::kParticleCount);
  }
  uint32_t particleArray() const {
    return mBase + fxswarm::kParticleArray;
  }
};

// Read-only lens over one particle record.
struct FxSwarmParticle {
  Core *mCore;
  uint32_t mBase;

  uint32_t worldXY() const {
    return mCore->mem_r32(mBase + fxswarm::kPartPosXY);
  }
  uint32_t worldZ() const {
    return mCore->mem_r32(mBase + fxswarm::kPartPosZ);
  }
  int32_t sizeMul() const {
    return mCore->mem_r16s(mBase + fxswarm::kPartSizeMul);
  }
};

// ------------------------------------------------------------------------------------------------
class FxSpriteSwarm {
public:
  // FUN_800281EC — draw a node's sprite cluster ONCE PER PARTICLE, each particle supplying its own
  // world position and its own size. a0 = the type-0x20 render node.
  static void emitPerParticle(Core *c);

  static void registerOverrides(Game *game);

private:
  // Load the PURE SCENE CAMERA (the rotation matrix + translation the scene pass parks in the
  // scratchpad) into the GTE's control registers, so the RTPS below projects world -> screen.
  static void loadSceneCameraToGte(Core *c);
};
