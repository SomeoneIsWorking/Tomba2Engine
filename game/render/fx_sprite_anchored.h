// game/render/fx_sprite_anchored.h — FUN_80027CB4, the SINGLE-ANCHOR member of the FUN_80027A4C
// world-anchored scaled-sprite family.
//
// See fx_sprite_anchored.cpp for the identification evidence and for what separates this emitter
// from its two siblings. This header holds only the typed LENS over the render node, plus a named
// constant for every field offset, so the body reads as "project this node's anchor and stamp its
// sprite cluster there" rather than as a run of mem_r32(base + 0x2C). The five-word scratchpad
// handoff every member of the family shares lives in fx_sprite_publish.h.
//
// The one write accessor is deliberately a ONE-LINER: tools/port_check.py harvests a lens setter's
// mem_wN width by regex, so `node.setRecordTail(v)` counts as exactly the store it performs. A
// setter that grew a second statement or a nested brace would silently stop counting.
#pragma once
#include <stdint.h>
#include "core.h"
#include "fx_sprite_publish.h"

class Game;

// ------------------------------------------------------------------------------------------------
// The type-0x20 RENDER NODE this member hangs off. The field object walk dispatches the node's own
// render fn at +0x18; for this member that fn is 0x80027CB4. Unlike the swarm sibling there is no
// particle array — the node's OWN world transform is the one and only anchor, so the whole cluster
// is stamped once, at one place on screen.
namespace fxanchored {
constexpr uint32_t kWorldAnchorXY = 0x2C;  // u32 — packed world VX (lo16) | VY (hi16)
// The Z word does double duty, exactly the way the swarm's particle records pack their size into
// the high half: the emitter feeds the WHOLE 32-bit word to the GTE's VZ0 (which consumes only the
// low 16 bits) and reads the high half back separately as the OT bucket bias.
constexpr uint32_t kWorldAnchorZ  = 0x30;  // u32 — world VZ in the low 16 bits
constexpr uint32_t kOtBias        = 0x32;  // s16 — bias added to the OT bucket key (that word's hi half)
constexpr uint32_t kRecordHead    = 0x34;  // u32 — head of the 8-byte sprite-record list (writer a0)
constexpr uint32_t kRecordTail    = 0x38;  // u32 — tail the writer returns; the ONLY guest field written
constexpr uint32_t kClut          = 0x44;  // u16 — texture CLUT id     } together the writer's a1
constexpr uint32_t kTexturePage   = 0x46;  // u16 — texture page bits   }
}  // namespace fxanchored

struct FxAnchoredNode {
  Core* mCore;
  uint32_t mBase;

  uint32_t worldAnchorXY() const { return mCore->mem_r32(mBase + fxanchored::kWorldAnchorXY); }
  uint32_t worldAnchorZ()  const { return mCore->mem_r32(mBase + fxanchored::kWorldAnchorZ); }
  int32_t  otBias()        const { return mCore->mem_r16s(mBase + fxanchored::kOtBias); }
  uint32_t recordHead()    const { return mCore->mem_r32(mBase + fxanchored::kRecordHead); }
  uint32_t clut()          const { return mCore->mem_r16(mBase + fxanchored::kClut); }
  uint32_t texturePage()   const { return mCore->mem_r16(mBase + fxanchored::kTexturePage); }

  void setRecordTail(uint32_t v) { mCore->mem_w32(mBase + fxanchored::kRecordTail, v); }
};

// ------------------------------------------------------------------------------------------------
// The SINGLE-ANCHOR half of the family: one cluster, stamped at the node's own world position.
// FUN_80027E5C — identical in shape, differing only by a `* (u8)node+6 >> 4` on the scale — belongs
// in this class as a second method when it is ported.
class FxSpriteAnchored {
public:
  // FUN_80027CB4 — project the node's own world anchor and stamp its sprite cluster there at the
  // UNIFORM depth-derived scale (no per-node and no per-particle size multiplier: the sprite's size
  // is purely a function of how far away it is). a0 = the type-0x20 render node.
  static void emitUniformScale(Core* c);

  static void registerOverrides(Game* game);

private:
  // Load the PURE SCENE CAMERA (the rotation matrix + translation the scene pass parks in the
  // scratchpad) into the GTE's control registers, so the RTPS below projects world -> screen.
  static void loadSceneCameraToGte(Core* c);
};
