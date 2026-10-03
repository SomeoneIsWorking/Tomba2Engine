// game/render/fx_water_jet.cpp — native display producer for guest FUN_8013D454, the A00 water jet.
//
// One controller, two mutually exclusive branches, selected by the node's own signed mode word. Both
// are owned here because they are the SAME controller: the mesh branch used to be a bounded guest-GTE
// replay (game/render/guest_gte_water_jet.cpp, deleted) and the sprite branch lived among the
// FUN_800328EC family in fx_sprite.cpp, so one effect's two halves sat in two modules and each half's
// comment described the other as missing.
//
// ── RE, from the image ───────────────────────────────────────────────────────────────────────────
// FUN_8013D454 (A00.BIN, loaded at the MODE slot 0x80108F9C) is 78 instructions and reads:
//
//   addiu sp, sp, -0x48 ; sp+0x10..sp+0x27 = a 24-byte COPY of the record-list table 0x8010A058
//                         (lui v0,0x8011 ; addiu t3,v0,-0x5FA8)
//   lhu  node+0x2E -> sh sp+0x28 ; lhu node+0x32 -> sh sp+0x2A ; lhu node+0x36 -> sh sp+0x2C
//                         — the world anchor, three SEPARATE s16s, not the packed pair at 0x2C
//   lhu  node+0x60 ; sll 16 ; sra 16   -> s0 = the MODE (signed)
//   lhu  node+0x62 ; sll 16 ; sra 16   -> s1 ; and sra 20 -> the uniform scale byte
//   beqz s0, <the zero-mode branch>
//
// MESH BRANCH (s0 != 0), the whole of it:
//
//   sb  scaleByte -> sp+0x30, sp+0x31, sp+0x32        (three IDENTICAL scale bytes)
//   a0 = &sp+0x28 (anchor)  a1 = &sp+0x30 (scale bytes)  a2 = node+0x54 (Euler angles)
//   jal FUN_800318A0                                   -> the shared local-transform publish
//   a1 = 0 ; a2 = -250 ; a3 = 0 ; a0 = *(u32*)(table + s0*4)
//   jal FUN_80027768(mesh, clutRow 0, sortBias -250, uScroll 0)
//
// So the mesh is chosen by MODE from the table at 0x8010A058 — six 36-byte-record packed meshes — and
// both non-zero modes the effect uses (1 and 2) are the same white translucent two-quad jet strip at
// different heights. FUN_800318A0's contract (world anchor, three unsigned scale bytes each << 2, three
// Euler angles) is already owned and documented in obj_model_view.cpp; the transform is rebuilt here in
// host memory through MeshQuads and composed with the native scene camera, so the jet expands correctly
// under widescreen and is reprojected under the lerped camera.
//
// WHY depthCue = 0 IS A STATEMENT, NOT A DEFAULT. This controller does NOT publish IR0 (0x1F800090),
// unlike FUN_8013ED08 which zeroes it explicitly. The guest therefore inherits whatever the previous
// display producer left there, and a producer that has not RE'd that inheritance cannot invent it. The
// identity is the one reading that cannot be wrong about an unmeasured value: with IR0 = 0 the
// writer's DPCT/DPCS cue returns each record's authored colour unchanged and no far-colour read
// happens. A non-zero inherited IR0 would tint the jet by whatever surface it passes over; that is
// recorded as the open question rather than guessed at.
//
// NOT A TAP: every input is the game's own pre-quantisation state (two node words for the anchor, two
// for mode and scale, three for the angles, one table entry) plus the node's own table in the overlay.
// No GTE register, scratch transform, packet, ordering table or guest-execution output is read, and the
// camera arrives through projComposeObjectHost, so the jet interpolates with the rest of the frame
// instead of stepping at the logic rate. The guest's own FUN_8013D454 still executes underneath and
// still owns every store it makes.
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

constexpr uint32_t kGuestControllerAddr = 0x8013D454u;

// The world anchor: three SEPARATE s16s, so the Y and Z are not the packed pair at 0x2C.
constexpr uint32_t kJetAnchorX = 0x2Eu;
constexpr uint32_t kJetAnchorY = 0x32u;
constexpr uint32_t kJetAnchorZ = 0x36u;

// (s16)node+0x60 selects the branch and indexes the record-list table; (s16)node+0x62 is the scale.
constexpr uint32_t kJetMode = 0x60u;
constexpr uint32_t kJetScale = 0x62u;
constexpr uint32_t kJetAngles = 0x54u;

// The record-list table, six packed meshes. The zero entry is the sprite's model; the mesh branch
// indexes it with the MODE itself, so modes 1 and 2 reach the two jet strips.
constexpr uint32_t kJetModelTable = 0x8010A058u;
constexpr int kJetModelTableEntries = 6;

// The writer arguments, all literal `li` in the guest body rather than anything node-derived.
constexpr int kJetWriterUScroll = 0;
constexpr int kJetWriterClutRowBias = 0;
constexpr int32_t kJetSortBias = -250;
constexpr int32_t kJetCueOff = 0;
const int32_t kJetNoFarColour[3] = {0, 0, 0};

// The zero branch: FUN_800329E0(4) programs DQA 4 rather than the family's usual 6, and the OT key
// carries the same -64 through both the gate and the published depth.
constexpr int kJetSpriteDqa = 4;
constexpr int kJetSpriteBias = -64;

constexpr int32_t kIdentity[3][3] = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}};

} // namespace

// FUN_8013D454, MESH branch — one of the two non-zero modes, drawn as a packed mesh.
void Render::waterJetMeshRender(uint32_t node) {
  Core *c = mCore;
  const int32_t mode = (int16_t)c->mem_r16(node + kJetMode);
  if (mode <= 0 || mode >= kJetModelTableEntries) {
    // The guest indexes the table with the signed mode itself, so a negative or out-of-range one
    // would read outside it. No mode in the effect is meant to do that; nothing is drawn rather than
    // a mesh read from wherever the index landed.
    lucent::debug("waterjetmesh",
                  "f{} node={:08X} mode={} is outside the {} entry record table — nothing drawn",
                  c->game->gpu.s_frame,
                  node,
                  mode,
                  kJetModelTableEntries);
    return;
  }
  const uint32_t mesh = c->mem_r32(kJetModelTable + (uint32_t)mode * 4u);
  if (!mesh) {
    lucent::debug("waterjetmesh",
                  "f{} node={:08X} mode={} mesh=00000000 — controller requested no records",
                  c->game->gpu.s_frame,
                  node,
                  mode);
    return;
  }

  EffectPoints live;
  live.n = 1;
  live.valid[0] = true;
  live.x[0] = c->mem_r16s(node + kJetAnchorX);
  live.y[0] = c->mem_r16s(node + kJetAnchorY);
  live.z[0] = c->mem_r16s(node + kJetAnchorZ);
  const EffectPoints &points = mEffectLerp.resolve(c, node, live);
  const float position[3] = {(float)points.x[0], (float)points.y[0], (float)points.z[0]};

  // The guest writes ONE byte to all three scale slots, so the jet's column scales are uniform.
  const int16_t scaleWord = (int16_t)c->mem_r16(node + kJetScale);
  const int32_t columnScale[3] = {
      (int32_t)(uint8_t)((int32_t)scaleWord >> 4) << 2,
      (int32_t)(uint8_t)((int32_t)scaleWord >> 4) << 2,
      (int32_t)(uint8_t)((int32_t)scaleWord >> 4) << 2,
  };

  const int16_t angleX = c->mem_r16s(node + kJetAngles + 0u);
  const int16_t angleY = c->mem_r16s(node + kJetAngles + 2u);
  const int16_t angleZ = c->mem_r16s(node + kJetAngles + 4u);
  int32_t rotation[3][3];
  MeshQuads::rotmat(c, angleX, angleY, angleZ, rotation);
  float objectRotation[3][3];
  MeshQuads::composeScaled(rotation, kIdentity, columnScale, objectRotation);

  EObjXform object;
  projComposeObjectHost(objectRotation, position, &object);
  projSetActive(&object);

  ProducerScope producerScope(&c->rsub.producerScope, kGuestControllerAddr, "waterJetMeshRender");
  ObjScope objectScope(c, node);
  const MeshOtBias ot{/*known=*/true, /*bias=*/kJetSortBias};
  const MeshQuadStyle style{kJetWriterUScroll, kJetNoFarColour, kJetCueOff};
  float bbox[4] = {1e9f, 1e9f, -1e9f, -1e9f};
  const int drawn = meshQuadRecordsEmit(mesh, style, ot, bbox);
  projClearActive();

  lucent::debug("waterjetmesh",
                "f{} t={:.2f} node={:08X} mode={} mesh={:08X} pos=({:.1f},{:.1f},{:.1f}) ang=({},{},{}) "
                "scale={} quads={} screen=[{:.1f},{:.1f}]..[{:.1f},{:.1f}] interp=host",
                c->game->gpu.s_frame,
                (double)fps60(*c->game).mT,
                node,
                mode,
                mesh,
                (double)position[0],
                (double)position[1],
                (double)position[2],
                angleX,
                angleY,
                angleZ,
                columnScale[0],
                drawn,
                (double)(drawn ? bbox[0] : 0.0f),
                (double)(drawn ? bbox[1] : 0.0f),
                (double)(drawn ? bbox[2] : 0.0f),
                (double)(drawn ? bbox[3] : 0.0f));
}

// FUN_8013D454, SPRITE branch — mode zero. The same anchor the mesh branch uses, one record list out of
// the same table (entry 0), and the -64 bias the guest applies to both the OT gate and the depth.
void Render::waterJetSpriteRender(uint32_t node) {
  Core *c = mCore;
  const uint32_t numer = (uint16_t)c->mem_r16(node + kJetScale);
  AltSprite a;
  a.node = node;
  a.anchorX = kJetAnchorX;
  a.dqa = kJetSpriteDqa;
  a.gateBias = kJetSpriteBias;
  a.depthBias = kJetSpriteBias;
  a.rec0 = c->mem_r32(kJetModelTable);
  a.numerX = numer;
  a.numerY = numer;
  altSpriteEmit(a);
}

// FUN_8013D454 — the whole controller: the mode word picks the branch, and it is the SAME signed word
// the guest branches on, so exactly one of the two halves ever draws.
void Render::waterJetRender(uint32_t node) {
  if ((int16_t)mCore->mem_r16(node + kJetMode) == 0) {
    waterJetSpriteRender(node);
    return;
  }
  waterJetMeshRender(node);
}