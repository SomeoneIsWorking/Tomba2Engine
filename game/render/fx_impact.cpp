// game/render/fx_impact.cpp — native display-pass producer for FUN_800288AC, the MESH half of
// FUN_80033080's weapon-impact burst.
//
// The composite node render function calls two independent emitters: FUN_80027E5C supplies the
// white sprite flash (Render::impactBurstRender), then FUN_800288AC supplies the expanding blue-white
// packed-mesh plume implemented here. The latter disappeared when the old fx_mesh GTE-register tap
// was correctly deleted; this producer rebuilds the controller transform from persistent node and
// animation-script state instead.
//
// Ground truth: generated/shard_5.c gen_func_800288AC and gen_func_80027768, decoded in
// docs/re/impact-plume-288ac.md. No generated body, guest packet, scratchpad transform, or GTE register
// is read to make this picture. The producer is read-only and runs under the fps60-lerped display-pass
// camera through projComposeObjectHost.
#include "core.h"
#include "game.h"
#include "mesh_quads.h"
#include "projection.h"
#include "render.h"
#include "render_internal.h"

#include <cstdint>
#include <lucent/log.h>

namespace {

constexpr uint32_t kGuestControllerAddr = 0x800288ACu;

constexpr uint32_t kNodeUScroll = 0x29u;    // u8 passed to FUN_80027768 as a3
constexpr uint32_t kNodeAnchor = 0x2Cu;     // s16 x/y/z world anchor
constexpr uint32_t kNodeSortBias = 0x32u;   // s16 passed to FUN_80027768 as a2
constexpr uint32_t kNodeAnimScript = 0x3Cu; // u32 -> current four-byte animation record
constexpr uint32_t kNodeAngles = 0x48u;     // s16 x3 Euler angles
constexpr uint32_t kNodeMesh = 0x50u;       // u32 packed 36-byte-record mesh

constexpr uint32_t kScaleX = 0u; // animation record: three u8 column scales, each << 2
constexpr uint32_t kScaleY = 1u;
constexpr uint32_t kScaleZ = 2u;
constexpr uint32_t kAttr = 3u; // bit7 last, bit6 depth cue, low nibble CLUT-row bias otherwise
constexpr uint8_t kCueEnabled = 0x40u;
constexpr uint8_t kCueMask = 0x3Fu;
constexpr uint8_t kClutRowMask = 0x0Fu;
constexpr int32_t kCueOne = 4096;
constexpr int kCueShift = 6;

const int32_t kIdentity[3][3] = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}};
const int32_t kBlack[3] = {0, 0, 0};

} // namespace

// FUN_800288AC — one packed-mesh copy at the impact node's anchor. The surrounding type-0x20 walk
// already scopes this emission to the node's indirectly-dispatched composite FUN_80033080, which is
// also where the guest producer leg attributes both halves of the burst.
void Render::impactPlumeRender(uint32_t node) {
  Core *c = mCore;
  const uint32_t script = c->mem_r32(node + kNodeAnimScript);
  if (!script) {
    lucent::debug("impactfx", "f{} node={:08X} script=00000000 — no mesh plume", c->game->gpu.s_frame, node);
    return;
  }

  const uint8_t attr = c->mem_r8(script + kAttr);
  const int32_t columnScale[3] = {(int32_t)c->mem_r8(script + kScaleX) << 2,
                                  (int32_t)c->mem_r8(script + kScaleY) << 2,
                                  (int32_t)c->mem_r8(script + kScaleZ) << 2};
  const int16_t angleX = c->mem_r16s(node + kNodeAngles + 0u);
  const int16_t angleY = c->mem_r16s(node + kNodeAngles + 2u);
  const int16_t angleZ = c->mem_r16s(node + kNodeAngles + 4u);
  const float anchor[3] = {(float)c->mem_r16s(node + kNodeAnchor + 0u),
                           (float)c->mem_r16s(node + kNodeAnchor + 2u),
                           (float)c->mem_r16s(node + kNodeAnchor + 4u)};

  int32_t rotation[3][3];
  MeshQuads::rotmat(c, angleX, angleY, angleZ, rotation);
  float objectRotation[3][3];
  MeshQuads::composeScaled(rotation, kIdentity, columnScale, objectRotation);

  EObjXform xform;
  projComposeObjectHost(objectRotation, anchor, &xform);
  projSetActive(&xform);

  const int32_t cue = (attr & kCueEnabled) ? kCueOne - ((int32_t)(attr & kCueMask) << kCueShift) : 0;
  const int clutRowBias = (attr & kCueEnabled) ? 0 : (int)(attr & kClutRowMask);
  const int uScroll = (int)c->mem_r8(node + kNodeUScroll);
  const MeshOtBias ot{/*known=*/true, /*bias=*/(int32_t)c->mem_r16s(node + kNodeSortBias)};
  const uint32_t mesh = c->mem_r32(node + kNodeMesh);

  ObjScope objScope(c, node);
  float bbox[4] = {1e9f, 1e9f, -1e9f, -1e9f};
  MeshQuadStyle style{uScroll, kBlack, cue};
  style.clutRowBias = clutRowBias;
  const int drawn = meshQuadRecordsEmit(mesh, style, ot, bbox);
  projClearActive();

  lucent::debug("impactfx",
                "f{} t={:.2f} fn={:08X} node={:08X} script={:08X} attr={:02X} mesh={:08X} "
                "scale=({},{},{}) angles=({},{},{}) anchor=({:.0f},{:.0f},{:.0f}) "
                "bias={} u={} clutrow={} cue={} quads={} screen=[{:.1f},{:.1f}]..[{:.1f},{:.1f}]",
                c->game->gpu.s_frame,
                (double)c->game->fps60.mT,
                kGuestControllerAddr,
                node,
                script,
                attr,
                mesh,
                columnScale[0],
                columnScale[1],
                columnScale[2],
                angleX,
                angleY,
                angleZ,
                (double)anchor[0],
                (double)anchor[1],
                (double)anchor[2],
                ot.bias,
                uScroll,
                clutRowBias,
                cue,
                drawn,
                (double)(drawn ? bbox[0] : 0.0f),
                (double)(drawn ? bbox[1] : 0.0f),
                (double)(drawn ? bbox[2] : 0.0f),
                (double)(drawn ? bbox[3] : 0.0f));
}
