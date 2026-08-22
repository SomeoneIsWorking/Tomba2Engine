// game/render/fx_swing.cpp — native producer for FUN_8002A834's weapon-swing starburst.
//
// The controller owns ten four-byte particle records at node+0x50. Each record supplies three Euler
// angles (bytes << 4) and one uniform scale (byte << 2). FUN_8002A834 builds that transform, anchors
// it at node+0x2C, composes it with the camera, and calls the packed-mesh writer FUN_80027768 for the
// fixed model at 0x8009FB0C. The writer receives the node's s16 sort bias at +0x32, no CLUT/U bias,
// IR0=0xFFF, and a far colour selected by the owning object's type byte.
//
// This display-pass producer rebuilds exactly that contract from the controller's own persistent
// state. It does not recover a transform from GTE registers, run a guest body, read guest packets, or
// mutate guest memory. Consequently it reprojects under the native fps60 camera and covers every
// weapon swing that uses this controller rather than one captured call site.
#include "core.h"
#include "game.h"
#include "mesh_quads.h"
#include "producer_scope.h"
#include "projection.h"
#include "render.h"
#include "render_internal.h"
#include <cstdint>
#include <lucent/log.h>

namespace {

constexpr uint32_t kGuestControllerAddr = 0x8002A834u;
constexpr uint32_t kNodeOwner = 0x10u;       // u32 -> object whose type selects the far colour
constexpr uint32_t kNodeAnchor = 0x2Cu;      // s16 x/y/z world anchor
constexpr uint32_t kNodeSortBias = 0x32u;    // s16 passed to FUN_80027768 as a2
constexpr uint32_t kNodeParticles = 0x50u;   // ten {u8 angleX, angleY, angleZ, uniformScale}
constexpr uint32_t kOwnerEffectType = 0x02u; // u8 index into kFarColourTable

constexpr uint32_t kFarColourTable = 0x800A1FC4u; // four bytes per owner effect type
constexpr uint32_t kStarburstMesh = 0x8009FB0Cu;
constexpr int kParticleCount = 10;
constexpr uint32_t kParticleStride = 4u;
constexpr int32_t kCueFull = 0xFFF;
constexpr int kUScroll = 0;
constexpr int kClutRowBias = 0;

const int32_t kIdentity[3][3] = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}};

} // namespace

// Guest FUN_8002A834's ten-copy weapon-charge starburst, rebuilt from the controller's persistent
// particle records and anchor. Read-only; dispatched from the type-0x20 display walk.
void Render::swingStarburstRender(uint32_t node) {
  Core *c = mCore;
  const uint32_t owner = c->mem_r32(node + kNodeOwner);
  if (!owner) {
    lucent::debug("swingfx", "f{} node={:08X} owner=00000000 — no starburst", c->game->gpu.s_frame, node);
    return;
  }

  const uint8_t effectType = c->mem_r8(owner + kOwnerEffectType);
  const uint32_t farEntry = kFarColourTable + (uint32_t)effectType * 4u;
  const int32_t farColour[3] = {(int32_t)c->mem_r8(farEntry + 0u) << 4,
                                (int32_t)c->mem_r8(farEntry + 1u) << 4,
                                (int32_t)c->mem_r8(farEntry + 2u) << 4};
  const float anchor[3] = {(float)c->mem_r16s(node + kNodeAnchor + 0u),
                           (float)c->mem_r16s(node + kNodeAnchor + 2u),
                           (float)c->mem_r16s(node + kNodeAnchor + 4u)};
  const MeshOtBias ot{/*known=*/true, /*bias=*/(int32_t)c->mem_r16s(node + kNodeSortBias)};

  ProducerScope producerScope(&c->rsub.producerScope, kGuestControllerAddr, "swingStarburstRender");
  ObjScope objScope(c, node);
  int drawn = 0;
  float bbox[4] = {1e9f, 1e9f, -1e9f, -1e9f};
  for (int i = 0; i < kParticleCount; i++) {
    const uint32_t particle = node + kNodeParticles + (uint32_t)i * kParticleStride;
    const int16_t angleX = (int16_t)((uint16_t)c->mem_r8(particle + 0u) << 4);
    const int16_t angleY = (int16_t)((uint16_t)c->mem_r8(particle + 1u) << 4);
    const int16_t angleZ = (int16_t)((uint16_t)c->mem_r8(particle + 2u) << 4);
    const int32_t scale = (int32_t)c->mem_r8(particle + 3u) << 2;

    int32_t rot[3][3];
    MeshQuads::rotmat(c, angleX, angleY, angleZ, rot);
    const int32_t columnScale[3] = {scale, scale, scale};
    float objectRot[3][3];
    MeshQuads::composeScaled(rot, kIdentity, columnScale, objectRot);

    EObjXform xform;
    projComposeObjectHost(objectRot, anchor, &xform);
    projSetActive(&xform);
    MeshQuadStyle style{kUScroll, farColour, kCueFull};
    style.clutRowBias = kClutRowBias;
    drawn += meshQuadRecordsEmit(kStarburstMesh, style, ot, bbox);
    projClearActive();
  }

  lucent::debug("swingfx",
                "f{} t={:.2f} node={:08X} owner={:08X} type={:02X} bias={} "
                "anchor=({:.0f},{:.0f},{:.0f}) copies={} quads={} "
                "screen=[{:.1f},{:.1f}]..[{:.1f},{:.1f}]",
                c->game->gpu.s_frame,
                (double)c->game->fps60.mT,
                node,
                owner,
                effectType,
                ot.bias,
                (double)anchor[0],
                (double)anchor[1],
                (double)anchor[2],
                kParticleCount,
                drawn,
                (double)(drawn ? bbox[0] : 0.0f),
                (double)(drawn ? bbox[1] : 0.0f),
                (double)(drawn ? bbox[2] : 0.0f),
                (double)(drawn ? bbox[3] : 0.0f));
}
