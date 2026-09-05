// game/render/fx_rigid_mesh.cpp — native display producer for A00's single rigid effect mesh
// (guest FUN_8013ED08).
//
// Generated ground truth (`overlay guest 0x8013ED08`) is compact and fully attributable:
//
//   *(u32*)0x1F800090 = 0;
//   FUN_800318A0(node+0x2C, node+0x54, node+0x48);
//   FUN_80027768(*(u32*)(node+0x50), 0, (s16)*(node+0x32), (u8)node[7]);
//
// FUN_800318A0's contract is already owned by ObjModelView and documented in obj_model_view.cpp:
// world anchor, three unsigned per-column scale bytes (each <<2), then three Euler angles. This
// producer rebuilds the same local transform in host memory through MeshQuads and composes it with
// the native scene camera. It therefore expands correctly under widescreen and is reprojected under
// the interpolated camera instead of consuming the guest's integer GTE result. The node anchor also
// passes through EffectLerp, the engine's existing host-only interpolation owner for effect points.
//
// Material state is not inherited: this controller explicitly publishes IR0=0, so the shared
// writer's DPCT/DPCS stage is the identity and the otherwise-live GTE far colour is irrelevant. No
// guest register, scratch transform, packet, ordering table, or guest-execution output is read.
#include "core.h"
#include "effect_lerp.h"
#include "fps60.h"
#include "game.h"
#include "mesh_quads.h"
#include "producer_scope.h"
#include "projection.h"
#include "render.h"
#include "render_internal.h"
#include <cstdint>
#include <lucent/log.h>

namespace {

constexpr uint32_t kGuestControllerAddr = 0x8013ED08u;
constexpr uint32_t kNodeUScroll = 0x07u;
constexpr uint32_t kNodeWorldPos = 0x2Cu;
constexpr uint32_t kNodeSortBias = 0x32u;
constexpr uint32_t kNodeAngles = 0x48u;
constexpr uint32_t kNodeMesh = 0x50u;
constexpr uint32_t kNodeScaleBytes = 0x54u;
constexpr int32_t kCueIdentity = 0;
constexpr int32_t kFarColourIrrelevant[3] = {0, 0, 0};
constexpr int32_t kIdentity[3][3] = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}};

} // namespace

// FUN_8013ED08 — one rigid packed-mesh controller, rebuilt from its persistent node state.
void Render::rigidMeshEffectRender(uint32_t node) {
  Core *c = mCore;
  const uint32_t mesh = c->mem_r32(node + kNodeMesh);
  if (!mesh) {
    lucent::debug(
        "rigidmeshfx", "f{} node={:08X} mesh=00000000 — controller requested no records", c->game->gpu.s_frame, node);
    return;
  }

  EffectPoints live;
  live.n = 1;
  live.valid[0] = true;
  live.x[0] = c->mem_r16s(node + kNodeWorldPos + 0u);
  live.y[0] = c->mem_r16s(node + kNodeWorldPos + 2u);
  live.z[0] = c->mem_r16s(node + kNodeWorldPos + 4u);
  const EffectPoints &points = mEffectLerp.resolve(c, node, live);
  const float position[3] = {(float)points.x[0], (float)points.y[0], (float)points.z[0]};

  const int16_t angleX = c->mem_r16s(node + kNodeAngles + 0u);
  const int16_t angleY = c->mem_r16s(node + kNodeAngles + 2u);
  const int16_t angleZ = c->mem_r16s(node + kNodeAngles + 4u);
  int32_t rotation[3][3];
  MeshQuads::rotmat(c, angleX, angleY, angleZ, rotation);

  const int32_t columnScale[3] = {
      (int32_t)c->mem_r8(node + kNodeScaleBytes + 0u) << 2,
      (int32_t)c->mem_r8(node + kNodeScaleBytes + 1u) << 2,
      (int32_t)c->mem_r8(node + kNodeScaleBytes + 2u) << 2,
  };
  float objectRotation[3][3];
  MeshQuads::composeScaled(rotation, kIdentity, columnScale, objectRotation);

  EObjXform object;
  projComposeObjectHost(objectRotation, position, &object);
  projSetActive(&object);

  ProducerScope producerScope(&c->rsub.producerScope, kGuestControllerAddr, "rigidMeshEffectRender");
  ObjScope objectScope(c, node);
  const MeshOtBias ot{/*known=*/true, /*bias=*/(int32_t)c->mem_r16s(node + kNodeSortBias)};
  const MeshQuadStyle style{(int)c->mem_r8(node + kNodeUScroll), kFarColourIrrelevant, kCueIdentity};
  float bbox[4] = {1e9f, 1e9f, -1e9f, -1e9f};
  const int drawn = meshQuadRecordsEmit(mesh, style, ot, bbox);
  projClearActive();

  lucent::debug("rigidmeshfx",
                "f{} t={:.2f} node={:08X} mesh={:08X} pos=({:.1f},{:.1f},{:.1f}) "
                "ang=({},{},{}) scale=({},{},{}) u={} bias={} quads={} "
                "screen=[{:.1f},{:.1f}]..[{:.1f},{:.1f}]",
                c->game->gpu.s_frame,
                (double)fps60(*c->game).mT,
                node,
                mesh,
                (double)position[0],
                (double)position[1],
                (double)position[2],
                angleX,
                angleY,
                angleZ,
                columnScale[0],
                columnScale[1],
                columnScale[2],
                style.uBias,
                ot.bias,
                drawn,
                (double)(drawn ? bbox[0] : 0.0f),
                (double)(drawn ? bbox[1] : 0.0f),
                (double)(drawn ? bbox[2] : 0.0f),
                (double)(drawn ? bbox[3] : 0.0f));
}
