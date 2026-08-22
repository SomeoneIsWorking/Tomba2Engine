// game/render/prop_quad.cpp — display-pass picture owner for the margin controller's GT4 prop quads.
//
// WidescreenMarginQuad::emit remains the byte-exact substrate override: it mirrors the guest's GTE,
// packet-pool and OT writes. pc_render deliberately does not consume that guest output. This producer
// rebuilds the same picture from the controller's persistent object/node inputs and the shared packed
// quad-record decoder, under the live/interpolated native camera. It is read-only: no GTE state,
// guest packet, ordering table, scratchpad transform, or generated body is read or written here.
#include "prop_quad.h"

#include "core.h"
#include "game.h"
#include "mesh_quads.h"
#include "projection.h"
#include "render.h"
#include "render_internal.h"

#include <lucent/log.h>

namespace {

constexpr uint32_t kObjectRenderKind = 3u;
constexpr uint32_t kObjectRenderSubKind = 5u;
constexpr uint32_t kObjectUBias = 7u;
constexpr uint32_t kObjectAnchor = 44u;
constexpr uint32_t kObjectSortBias = 50u;
constexpr uint32_t kObjectNode = 60u;
constexpr uint32_t kObjectAngles = 72u;
constexpr uint32_t kObjectRecords = 80u;
constexpr uint32_t kObjectFogBase = 86u;

constexpr uint32_t kNodeScaleX = 0u;
constexpr uint32_t kNodeScaleY = 1u;
constexpr uint32_t kNodeScaleZ = 2u;
constexpr uint32_t kNodeFlags = 3u;

const int32_t kIdentity[3][3] = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}};
const int32_t kNoFarColour[3] = {0, 0, 0};

} // namespace

void Render::propQuadRender(uint32_t object) {
  Core *c = mCore;
  const uint32_t node = c->mem_r32(object + kObjectNode);
  const uint32_t records = c->mem_r32(object + kObjectRecords);
  if (!node || !records) {
    lucent::debug(
        "propquad", "f{} obj={:08X} node={:08X} records={:08X} quads=0", c->game->gpu.s_frame, object, node, records);
    return;
  }

  const int16_t angleX = c->mem_r16s(object + kObjectAngles + 0u);
  const int16_t angleY = c->mem_r16s(object + kObjectAngles + 2u);
  const int16_t angleZ = c->mem_r16s(object + kObjectAngles + 4u);
  const int32_t columnScale[3] = {
      PropQuadRecipe::columnScale(c->mem_r8(node + kNodeScaleX)),
      PropQuadRecipe::columnScale(c->mem_r8(node + kNodeScaleY)),
      PropQuadRecipe::columnScale(c->mem_r8(node + kNodeScaleZ)),
  };
  const float anchor[3] = {
      (float)c->mem_r16s(object + kObjectAnchor + 0u),
      (float)c->mem_r16s(object + kObjectAnchor + 2u),
      (float)c->mem_r16s(object + kObjectAnchor + 4u),
  };

  int32_t rotation[3][3];
  MeshQuads::rotmat(c, angleX, angleY, angleZ, rotation);
  float objectRotation[3][3];
  MeshQuads::composeScaled(rotation, kIdentity, columnScale, objectRotation);
  EObjXform xform;
  projComposeObjectHost(objectRotation, anchor, &xform);
  projSetActive(&xform);

  MeshQuadStyle style{(int)c->mem_r8(object + kObjectUBias) << 5, kNoFarColour, 0};
  style.clutRowBias = (int)(c->mem_r8(node + kNodeFlags) & 0x0Fu);
  style.fogFromVertex0Y = true;
  style.fogBase = c->mem_r16s(object + kObjectFogBase);
  style.tpageOverride =
      PropQuadRecipe::tpageOverride(c->mem_r8(object + kObjectRenderKind), c->mem_r8(object + kObjectRenderSubKind));
  style.semiOverride = 1; // guest rgb0 code 0x3E: textured gouraud quad with semi-transparency enabled
  const MeshOtBias ot{/*known=*/true, /*bias=*/(int32_t)c->mem_r16s(object + kObjectSortBias)};

  ObjScope objScope(c, object);
  float bbox[4] = {1e9f, 1e9f, -1e9f, -1e9f};
  const int drawn = meshQuadRecordsEmit(records, style, ot, bbox);
  projClearActive();

  lucent::debug("propquad",
                "f{} obj={:08X} node={:08X} records={:08X} scale=({},{},{}) "
                "angles=({},{},{}) anchor=({:.0f},{:.0f},{:.0f}) bias={} u={} fog={} "
                "clutrow={} tpage={} quads={} screen=[{:.1f},{:.1f}]..[{:.1f},{:.1f}]",
                c->game->gpu.s_frame,
                object,
                node,
                records,
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
                style.uBias,
                style.fogBase,
                style.clutRowBias,
                style.tpageOverride,
                drawn,
                (double)(drawn ? bbox[0] : 0.0f),
                (double)(drawn ? bbox[1] : 0.0f),
                (double)(drawn ? bbox[2] : 0.0f),
                (double)(drawn ? bbox[3] : 0.0f));
}
