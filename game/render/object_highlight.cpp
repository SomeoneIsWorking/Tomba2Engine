// object_highlight.cpp — native display-pass picture for queue A's packed-mesh object highlight.
//
// The queue-A object walk calls this controller after the object's ordinary mesh (and, for the tether
// arm, after the tether) when the node type carries bit 0x40 or 0x80. The guest controller builds a
// thin, fixed mesh at 0x8009FAE8 around the object, with its lateral scale selected by that type byte.
// The original guest controller executes through the JIT and owns its register, scratchpad, GTE and
// packet writes. This module independently rebuilds the picture from persistent node
// fields in Render::fieldObjectsRender, under the lerped native camera. It reads no GTE output, packet,
// ordering table or guest-written scratch transform.
//
// Ground truth: the resident controller in authenticated executable/overlay evidence and packed-record writer in
// authenticated executable/overlay evidence; the exact symbols are recorded in docs/producers/0x8002AE0C.md.
#include "core.h"
#include "fps60.h"
#include "game.h"
#include "mesh_quads.h"
#include "object_highlight_policy.h"
#include "producer_scope.h"
#include "projection.h"
#include "render.h"
#include "render_internal.h"

#include <cstdint>
#include <lucent/log.h>

namespace {

constexpr uint32_t kGuestController = 0x8002AE0Cu;
constexpr uint32_t kMesh = 0x8009FAE8u;
constexpr uint32_t kFieldMode = 0x800BF870u;

constexpr uint32_t kAnchorX = 0x2Eu;
constexpr uint32_t kAnchorY = 0x32u;
constexpr uint32_t kAnchorZ = 0x36u;
constexpr uint32_t kYOffsetBase = 0x84u;
constexpr uint32_t kYOffsetTip = 0x86u;
constexpr uint32_t kAngles = 0x54u;

constexpr int kUScroll = 0;
const int32_t kIdentity[3][3] = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}};
const int32_t kBlack[3] = {0, 0, 0};

} // namespace

void Render::objectHighlightRender(uint32_t node, int32_t scaleInput) {
  Core *c = mCore;

  const int16_t angleX = c->mem_r16s(node + kAngles + 0u);
  const int16_t angleY = c->mem_r16s(node + kAngles + 2u);
  const int16_t angleZ = c->mem_r16s(node + kAngles + 4u);
  int32_t rotation[3][3];
  MeshQuads::rotmat(c, angleX, angleY, angleZ, rotation);

  const int32_t lateral = ObjectHighlightPolicy::lateralScale(scaleInput);
  const int32_t columnScale[3] = {lateral, 64, lateral};
  float objectRotation[3][3];
  MeshQuads::composeScaled(rotation, kIdentity, columnScale, objectRotation);

  // The Y adjustment is explicitly 16-bit guest arithmetic: the controller stores this expression
  // to a stack halfword before composing the transform. Narrow at the same boundary.
  const uint16_t yBits =
      (uint16_t)(c->mem_r16(node + kAnchorY) + c->mem_r16(node + kYOffsetTip) - c->mem_r16(node + kYOffsetBase));
  const float anchor[3] = {
      (float)c->mem_r16s(node + kAnchorX), (float)(int16_t)yBits, (float)c->mem_r16s(node + kAnchorZ)};

  EObjXform xform;
  projComposeObjectHost(objectRotation, anchor, &xform);
  projSetActive(&xform);

  const ObjectHighlightPolicy::Cue cue = ObjectHighlightPolicy::cue(c->mem_r8(kFieldMode));
  ProducerScope producerScope(&c->rsub.producerScope, kGuestController, "objectHighlightRender");
  ObjScope objScope(c, node);
  float bbox[4] = {1e9f, 1e9f, -1e9f, -1e9f};
  const int drawn =
      meshQuadRecordsEmit(kMesh, MeshQuadStyle{kUScroll, kBlack, cue.amount}, MeshOtBias{true, cue.sortBias}, bbox);
  projClearActive();

  lucent::debug("highlightfx",
                "f{} t={:.2f} node={:08X} arg={} scale={} mode={} cue={} bias={} "
                "angles=({},{},{}) anchor=({:.0f},{:.0f},{:.0f}) quads={} "
                "screen=[{:.1f},{:.1f}]..[{:.1f},{:.1f}]",
                c->game->gpu.s_frame,
                (double)fps60(*c->game).mT,
                node,
                scaleInput,
                lateral,
                c->mem_r8(kFieldMode),
                cue.amount,
                cue.sortBias,
                angleX,
                angleY,
                angleZ,
                (double)anchor[0],
                (double)anchor[1],
                (double)anchor[2],
                drawn,
                (double)(drawn ? bbox[0] : 0.0f),
                (double)(drawn ? bbox[1] : 0.0f),
                (double)(drawn ? bbox[2] : 0.0f),
                (double)(drawn ? bbox[3] : 0.0f));
}
