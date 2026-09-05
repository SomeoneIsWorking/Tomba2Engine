// game/render/fx_sprite_publish.h — the SCRATCHPAD HANDOFF shared by every member of the
// FUN_80027A4C world-anchored scaled-sprite family.
//
// The family (Tomba!2's small burning/glittering effects — the seaside torch and the hut-roof
// flames are the confirmed live case, docs/findings/render.md) is ONE packet writer, FUN_80027A4C,
// fed by three interchangeable EMITTER render-fns hung off a type-0x20 node at node+0x18:
//     0x80027CB4  scale = MAC0                     -> FxSpriteAnchored::emitUniformScale
//     0x80027E5C  scale = MAC0 * (u8)node+6 >> 4   -> (not yet ported; same class as the above)
//     0x800281EC  per-particle swarm               -> FxSpriteSwarm::emitPerParticle
// Whatever an emitter does to pick an anchor and a size, it hands the result over in these five
// scratchpad words and then calls the writer, which consumes them. They are the emitter's whole
// output contract: where on screen this sprite sits, how big it is, which OT bucket it belongs in,
// and how hard to depth-cue its colours. One contract, one home — the emitters differ ABOVE this
// line, never below it.
//
// The write accessors are deliberately ONE-LINERS: tools/dynamic differential evidence harvests a lens setter's
// mem_wN width by regex, so `publish.setScaleX(v)` counts as exactly the store it performs. A setter
// that grew a second statement or a nested brace would silently stop counting.
#pragma once
#include "core.h"
#include <stdint.h>

namespace fxpublish {
constexpr uint32_t kOtKey = 0x1F800080u;    // s32 — OT bucket key, or -1 = culled
constexpr uint32_t kScaleX = 0x1F800084u;   // s32 — horizontal pixel scale (16.16)
constexpr uint32_t kScaleY = 0x1F800088u;   // s32 — vertical pixel scale
constexpr uint32_t kScreenXY = 0x1F80008Cu; // packed screen anchor, straight from GTE SXY2
constexpr uint32_t kDepthCue = 0x1F800090u; // s32 — IR0 for the writer's DPCS colour cue
} // namespace fxpublish

struct FxSpritePublish {
  Core *mCore;

  void setOtKey(int32_t v) {
    mCore->mem_w32(fxpublish::kOtKey, (uint32_t)v);
  }
  void setScaleX(int32_t v) {
    mCore->mem_w32(fxpublish::kScaleX, (uint32_t)v);
  }
  void setScaleY(int32_t v) {
    mCore->mem_w32(fxpublish::kScaleY, (uint32_t)v);
  }
  void setScreenXY(uint32_t v) {
    mCore->mem_w32(fxpublish::kScreenXY, v);
  }
  void setDepthCue(int32_t v) {
    mCore->mem_w32(fxpublish::kDepthCue, (uint32_t)v);
  }

  int32_t otKey() const {
    return (int32_t)mCore->mem_r32(fxpublish::kOtKey);
  }
  int32_t scaleX() const {
    return (int32_t)mCore->mem_r32(fxpublish::kScaleX);
  }
};
