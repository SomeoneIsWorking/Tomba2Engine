// game/render/fx_backdrop_plane.cpp — the AREA-14 BACKDROP PLANE, guest FUN_80110CA4 (A0E overlay).
// Found by the 22-area nofx sweep, and it is also the missing producer kanban #48 split out of #42
// ("area 14 waterfall backdrop is GTE SCENE GEOMETRY with no native producer").
//
// NOT a sprite-family member. It programs no DQA and never calls FUN_800317CC, so SpriteAnchor does
// not apply; it is scene geometry — a textured gouraud grid.
//
// RE from ov_a0e_gen_80110CA4, statically verified 2026-07-28 with its verifier's corrections
// (docs/re/render-targets-static-re.md — read the ORIGINAL algorithm alongside the CORRECTED delta).
//
// WHAT IT DRAWS — two grids sharing one transform:
//   GRID A: a 12000 x 16800 model-unit WALL at local Z = 0 — 7 rows (y = -7200..7200 step 2400) x
//           10 columns (x = -6000..4800 step 1200) = 70 quads. Its V coordinate is MIRRORED about
//           y = 0 and its lower half is tinted dark blue, i.e. the plane is a surface and its own
//           REFLECTION, with a bright seam row at y == 0.
//   GRID B: 10 quads on the local Y = 0 plane extending to Z = 1200 — the ADDITIVE glow band along
//           that seam (guest code byte 0x3E + tpage 45's semi bits = B + F).
// Both scroll: U steps one 64-texel tile every 2 ticks, V one every 8, from the frame tick.
//
// THIS IS HALF THE RENDER FN. The guest tail-calls 0x801104D0 (a 440-line sprite-family body) with
// the same node, and that half is NOT ported — so this producer draws the plane and its glow band,
// not whatever that tail adds. Recorded as such in the port-map rather than claimed complete.
//
// ORDERING IS DELIBERATELY NOT THE GUEST'S. The guest builds an OT key by log-compressing AVSZ4 and
// then adds a ROW BIAS (+30 for the upper half, +40 for the lower) plus a 2045 clamp — authored sort
// order for a painter's algorithm with no depth buffer. This producer gives every vertex its real
// projected depth instead: engine owns ordering, real depth for 3D. The plane is flat, so real depth
// reproduces the intended layering without the bias.
//
// Read-only: the guest's packet-pool cursor and OT writes are allocation bookkeeping this producer
// does not need.
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include "render_internal.h"   // ObjScope / proj_pz_to_ord
#include "projection.h"        // EObjXform
#include "fx_node.h"          // FxNode — the walk-owned header lens this controller extends
#include "fx_sprite.h"         // SpriteAnchor — the spark half is FUN_80027A4C family
#include "mesh_quads.h"        // MeshQuads::rotmat — host-output 3x3 from three Euler angles
#include "cfg.h"
#include <cstdint>

namespace {

// The A0E backdrop controller's OWN view of its node. Note +0x48 is EULER ANGLES here, where the
// area-8 mote controller uses the same offset for a per-frame cube base — exactly the per-controller
// divergence FxNode refuses to paper over.
class PlaneNode : public FxNode {
public:
  using FxNode::FxNode;
  int32_t posX() const { return s16(0x2Cu); }
  int32_t posY() const { return s16(0x2Eu); }
  int32_t posZ() const { return s16(0x30u); }
  int32_t rotX() const { return s16(0x48u); }   // Euler angles, NOT a cube base
  int32_t rotY() const { return s16(0x4Au); }
  int32_t rotZ() const { return s16(0x4Cu); }
};

constexpr uint32_t kTickWord    = 0x1F80017Cu;  // the frame tick the scroll is derived from
constexpr int      kTpage       = 45;
constexpr int      kClut        = 16190;        // 0x3F3E

// Grid A extents, in model units.
constexpr int kRowY0 = -7200, kRowY1 = 7200, kRowStep = 2400;
constexpr int kColX0 = -6000, kColX1 = 6000,  kColStep = 1200;
constexpr int kBandZ = 1200;                    // grid B's depth on the local Y = 0 plane

// FUN_801104D0's spark pool (A0E overlay data).
constexpr uint32_t kSparkState    = 0x8012686Cu;  // u32 per slot: record list, non-zero = live
constexpr uint32_t kSparkPos      = 0x80125BECu;  // {u16 x, y, z} per slot
constexpr uint32_t kSparkClut     = 0x8011B224u;  // shared clut | tpage
constexpr int      kSparkSlots    = 200;
constexpr int      kSparkGateBias = -50;
constexpr int      kSparkDqa      = 6;

// The guest's screen bounding test is TWO SEPARATE "any vertex passes" tests, not a per-vertex
// conjunction, and it reads the packed screen words UNSIGNED so a negative coordinate reads as
// >= 32768 and fails. Reproduced with that exact shape.
inline bool anyUnder(const float v[4], int limit) {
  for (int i = 0; i < 4; i++) {
    const int s = (int)v[i];
    if (s >= 0 && s < limit) return true;
  }
  return false;
}

}  // namespace

void Render::fxBackdropPlaneRender(uint32_t node) {
  Core* c = mCore;

  // Scroll: U advances one 64-texel step every 2 ticks (cycle 8), V one every 8 ticks (cycle 32).
  // Computed once, before any geometry, and never mutated inside the loops.
  const uint32_t t2 = (uint32_t)c->mem_r16(kTickWord) >> 1;
  const int uBase = (int)((t2 & 3u) << 6);
  const int vBase = (int)(((t2 >> 2) & 3u) << 6);

  // The node's own orientation, composed with the scene camera. The guest does
  // rotmat(node+0x48) -> matMul(Rcam, Rnode) -> applyMatlv(nodePos) -> += camT, which is exactly
  // projComposeObjectHost(Rnode, nodePos).
  const PlaneNode pn(c, node);
  int32_t M[3][3];
  MeshQuads::rotmat(c, (int16_t)pn.rotX(), (int16_t)pn.rotY(), (int16_t)pn.rotZ(), M);
  float Robj[3][3];
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) Robj[i][j] = (float)M[i][j];
  const float Tobj[3] = { (float)pn.posX(), (float)pn.posY(), (float)pn.posZ() };
  EObjXform cam;
  projComposeObjectHost(Robj, Tobj, &cam);

  RenderQueue& rq = c->game->activeRq();
  ObjScope objScope(c, node);
  int drawnA = 0, drawnB = 0;

  // emit one quad from four model-space corners, with per-vertex UV and colour.
  auto emitQuad = [&](const int mx[4], const int my[4], const int mz[4],
                      const int us[4], const int vs[4],
                      const uint32_t cols[4], int semi) -> bool {
    ProjVtx pv[4];
    float px[4], py[4], depth[4];
    for (int k = 0; k < 4; k++) {
      cam.project((int16_t)mx[k], (int16_t)my[k], (int16_t)mz[k], &pv[k]);
      if (pv[k].sz <= 0) return false;    // the guest's GTE FLAG cull, in the form a float path has
      px[k] = pv[k].px; py[k] = pv[k].py;
      depth[k] = proj_pz_to_ord(pv[k].pz);
    }
    if (!anyUnder(px, 321) || !anyUnder(py, 241)) return false;   // the guest's two "any" tests

    unsigned char rr[4], gg[4], bb[4];
    for (int k = 0; k < 4; k++) {
      rr[k] = (unsigned char)(cols[k] & 0xFFu);
      gg[k] = (unsigned char)((cols[k] >> 8) & 0xFFu);
      bb[k] = (unsigned char)((cols[k] >> 16) & 0xFFu);
    }
    rq.drawWorldQuad(c, px, py, depth, us, vs, rr, gg, bb, (uint16_t)kTpage, (uint16_t)kClut,
                     semi, nullptr);
    return true;
  };

  // ---- GRID A: the wall and its mirrored reflection -------------------------------------------
  for (int y = kRowY0; y < kRowY1 + 1; y += kRowStep) {
    int col = 0;
    for (int x = kColX0; x < kColX1; x += kColStep, col++) {
      const int mx[4] = { x, x + kColStep, x, x + kColStep };
      const int my[4] = { y - kRowStep, y - kRowStep, y, y };
      const int mz[4] = { 0, 0, 0, 0 };

      // U by column parity — a continuous 64-texel tile spanning every TWO columns.
      int us[4], vs[4];
      if (col & 1) { us[0] = us[2] = uBase;      us[1] = us[3] = uBase + 32; }
      else         { us[0] = us[2] = uBase + 32; us[1] = us[3] = uBase + 63; }

      uint32_t cols[4];
      if (y <= 0) {                       // upper half: the surface itself
        vs[0] = vs[1] = vBase; vs[2] = vs[3] = vBase + 62;
        cols[0] = cols[1] = 0x00808080u;
        cols[2] = cols[3] = (y == 0) ? 0x00DFDFDFu : 0x00808080u;   // bright seam row
      } else {                            // lower half: the reflection — V mirrored, tinted dark blue
        vs[3] = vs[2] = vBase; vs[1] = vs[0] = vBase + 62;
        cols[0] = cols[1] = (y == kRowStep) ? 0x00808080u : 0x00201000u;
        cols[2] = cols[3] = 0x00201000u;
      }
      if (emitQuad(mx, my, mz, us, vs, cols, /*semi=*/0)) drawnA++;
    }
  }

  // ---- GRID B: the additive glow band along the seam -------------------------------------------
  int col2 = 0;
  for (int x = kColX0; x < kColX1; x += kColStep, col2++) {
    const int mx[4] = { x, x + kColStep, x, x + kColStep };
    const int my[4] = { 0, 0, 0, 0 };
    const int mz[4] = { 0, 0, kBandZ, kBandZ };
    int us[4], vs[4];
    if (col2 & 1) { us[0] = us[2] = uBase;      us[1] = us[3] = uBase + 32; }
    else          { us[0] = us[2] = uBase + 32; us[1] = us[3] = uBase + 63; }
    vs[0] = vs[1] = vBase; vs[2] = vs[3] = vBase + 62;
    const uint32_t cols[4] = { 0x00808080u, 0x00808080u, 0x00000000u, 0x00000000u };
    if (emitQuad(mx, my, mz, us, vs, cols, /*semi=*/1)) drawnB++;   // additive
  }

  if (cfg_dbg("fxplane"))
    cfg_logf("fxplane", "backdrop node=%08X uv=(%d,%d) gridA=%d/70 gridB=%d/10",
             node, uBase, vBase, drawnA, drawnB);

  // The guest render fn TAIL-CALLS 0x801104D0 with the same node; reproduce that here so one walk
  // dispatch owns the whole effect, exactly as the guest does.
  fxBackdropSparkRender(node);
}

// ── FUN_801104D0 (A0E overlay, area 14) — the backdrop's SPARK/DROPLET POOL ───────────────────────
// The tail half of FUN_80110CA4, and kanban #67. A fixed 200-slot pool of sprite particles drawn over
// the waterfall — spray, embers or motes depending on how you read the art.
//
// THE KEY OBSERVATION, and why this port is small where the gen body is 441 lines: that body both
// SIMULATES and DRAWS. Per live slot it integrates pos += vel and adds 25 to vel.y (gravity), writes
// both back, then projects and emits; the rest of its bulk is the SPAWN state machine, which is where
// all 34 of its PRNG draws live. A read-only producer reproduces NONE of that — the guest's own body
// still runs underneath and keeps the pool simulated, so this producer just reads the pool's current
// state and draws it. That is also why it needs no GuestRngMirror: the randomness is upstream of the
// state we read.
//
// Pool layout, all A0E overlay data, parallel and indexed by slot:
//   0x8012686C + i*4 : u32 record list — the slot's sprite, and NON-ZERO IS THE LIVE FLAG
//   0x80125BEC + i*8 : {u16 x, u16 y, u16 z} the particle's world position
//   0x8012622C + i*8 : {u16 vx, u16 vy, u16 vz} its velocity (guest-owned; we never touch it)
//   0x8011B224       : the shared clut|tpage word
// Gate bias is -50 and DQA is 6, both from the guest; IR0 is programmed 0, so the depth cue is the
// identity and record colours pass through.
void Render::fxBackdropSparkRender(uint32_t node) {
  Core* c = mCore;
  EObjXform cam; projComposeCamera(&cam);
  const uint32_t H = (uint32_t)cam.H;
  const uint32_t clutPage = c->mem_r32(kSparkClut);
  const float nearPz = proj_near_pz();

  RenderQueue& rq = c->game->activeRq();
  (void)rq;
  ObjScope objScope(c, node);
  int live = 0, drawn = 0;
  for (int i = 0; i < kSparkSlots; i++) {
    const uint32_t rec0 = c->mem_r32(kSparkState + (uint32_t)i * 4u);
    if (!rec0) continue;                        // free slot
    live++;
    const uint32_t p = kSparkPos + (uint32_t)i * 8u;
    ProjVtx pv;
    cam.project((int16_t)c->mem_r16(p), (int16_t)c->mem_r16(p + 2u), (int16_t)c->mem_r16(p + 4u), &pv);
    if (!SpriteAnchor::otKeyInRange(pv.sz, kSparkGateBias)) continue;   // the emitter's own skip

    // The authored near bias, in the units the rest of the family uses: the gate states it in OT
    // buckets and the bucket key is SZ3>>2, so one unit is four of view depth.
    float pz = pv.pz + (float)(4 * kSparkGateBias);
    if (pz < nearPz) pz = nearPz;

    const int32_t scale = SpriteAnchor::baseScale(H, pv.sz, kSparkDqa);
    spriteRecordsEmit(rec0, clutPage, pv.px, pv.py, proj_pz_to_ord(pz), scale, scale, 0, nullptr);
    drawn++;
  }

  if (cfg_dbg("fxplane"))
    cfg_logf("fxplane", "sparks node=%08X live=%d drawn=%d/%d", node, live, drawn, kSparkSlots);
}
