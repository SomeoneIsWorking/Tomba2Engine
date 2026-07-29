// game/render/fx_dotfield.cpp — the CAMERA-FOLLOWING DOT HAZE, guest FUN_801110BC (A0B overlay,
// area 11). Found by the 22-area nofx sweep; per claim C012 it currently draws nothing.
//
// NOT a sprite-family member. It programs no DQA and never calls FUN_800329E0 / FUN_800317CC, so
// SpriteAnchor::baseScale would be a fabricated number here and SpriteAnchor::otKeyInRange is not its
// gate — it has its own two gates (see below). Its primitive is an untextured flat screen-space
// square, not a textured record quad.
//
// RE from ov_a0b_gen_801110BC, statically verified 2026-07-28 including the verifier's corrections
// (docs/re/render-targets-static-re.md — read the ORIGINAL algorithm alongside the CORRECTED delta).
//
// WHAT IT IS: 513 opaque pure-white specks filling a 2048-unit cube centred one half-step ahead of
// the camera, wrapping infinitely on a world lattice keyed to the node's own anchor. The only depth
// cue is a size step — 2x2 px when nearer than SZ3 1536, else 1x1. No texture, no colour, no fade.
//
// THE "MATRIX PIPELINE" IS NOT REAL WORK — this function was mis-scoped as the hardest of the sweep
// targets on the strength of its callee list. 0x80084660 is SetRotMatrix (5 lw + 5 ctc2), 0x80084690
// is SetTransMatrix (3 lw + 3 ctc2), and gte_op(0x4A180001) is a plain RTPS sf=1 lm=0. The whole
// setup is algebraically "the scene camera pre-translated by the field origin", which is exactly what
// projComposeObjectHost(identity, origin) produces — so this needs no matrix helper and no gen body.
//
// ORDERING IS DELIBERATELY NOT THE GUEST'S. The guest prepends every dot into ONE fixed OT bucket
// (index 256) with no per-dot sorting, so its haze has no depth relationship to the scene at all.
// This producer gives each dot its REAL projected depth instead, which is the engine-owns-ordering
// rule (real depth for 3D) rather than a transcription of the guest's flat bucket. Registered as a
// deliberate divergence in spirit with the rest of pc_render.
//
// Read-only: the guest's packet-pool cursor, OT writes and scratchpad staging are all allocation
// bookkeeping this producer does not need. It notably does NOT write node+0x50 — the guest reads the
// LCG seed there and never writes it back, so the pattern is deterministic per node per frame.
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include "render_internal.h"   // ObjScope / proj_pz_to_ord
#include "projection.h"        // EObjXform
#include "fx_node.h"          // FxNode — the walk-owned header lens this controller extends
#include "cfg.h"
#include <cstdint>

namespace {

// The A0B dot-haze controller's OWN view of its node. Deriving from FxNode rather than adding these
// to it is the point: +0x2C is this controller's THREE SEPARATE s16 anchor (not the sprite family's
// packed VX|VY pair), and +0x50 is its LCG SEED where a sibling uses the same slot for a wind
// magnitude. Naming them here keeps that per-controller meaning visible at every call site.
class DotFieldNode : public FxNode {
public:
  using FxNode::FxNode;
  int32_t  latticeX() const { return s16(0x2Cu); }   // the world lattice the cube is keyed on
  int32_t  latticeY() const { return s16(0x2Eu); }
  int32_t  latticeZ() const { return s16(0x30u); }
  uint32_t lcgSeed()  const { return u32(0x50u); }   // READ ONLY — the guest never writes it back
};

constexpr uint32_t kDotLcgMulA   = 0x8011C030u;  // A0B overlay data: the LCG multiplier
constexpr uint32_t kCamViewRow3X = 0x1F800104u;  // third row of the scene view rotation = forward axis
constexpr uint32_t kCamEyeX      = 0x1F8000D2u;  // camera eye X, then Y at +4, Z at +8 (u16 reads)
constexpr int      kDotCount     = 513;          // loop runs 512..0 inclusive
constexpr int      kDotLattice   = 2048;         // the wrap cube edge
constexpr int      kDotHalfCube  = 1024;
constexpr int      kDotNearSz    = 1536;         // SZ3 under this -> 2x2 px, else 1x1
constexpr int      kDotMode      = 3;            // untextured flat

}  // namespace

void Render::fxDotFieldRender(uint32_t node) {
  Core* c = mCore;

  // STEP 1 — the camera-forward half-step. The view matrix's third row IS the camera forward axis in
  // world coords (4096 = 1.0); the guest's (sext16(v) << 11) >> 12 is exactly sext16(v) >> 1.
  const int fx = c->mem_r16s(kCamViewRow3X) >> 1;
  const int fy = c->mem_r16s(kCamViewRow3X + 2u) >> 1;
  const int fz = c->mem_r16s(kCamViewRow3X + 4u) >> 1;

  // STEP 2 — the field origin: half a cube behind the camera eye, pushed forward by the half-step, so
  // the cube straddles the camera. The eye is read UNSIGNED and the sum wraps in 16 bits, as stored.
  const int originX = (int16_t)(uint16_t)(c->mem_r16(kCamEyeX)      - kDotHalfCube + fx);
  const int originY = (int16_t)(uint16_t)(c->mem_r16(kCamEyeX + 4u) - kDotHalfCube + fy);
  const int originZ = (int16_t)(uint16_t)(c->mem_r16(kCamEyeX + 8u) - kDotHalfCube + fz);

  // STEP 3 — the wrap deltas. local = (rand + nodeAnchor - origin) & 2047 means the cloud sits on a
  // WORLD lattice keyed to the node, so it does not swim as the camera moves — it wraps.
  const DotFieldNode n(c, node);
  const int dx = n.latticeX() - originX;
  const int dy = n.latticeY() - originY;
  const int dz = n.latticeZ() - originZ;

  // STEP 4 — the field camera: the scene camera pre-translated by the origin. Identity rotation at
  // the GTE's 4096 = 1.0 scale, translation = the origin.
  const float Robj[3][3] = { { 4096.0f, 0.0f, 0.0f }, { 0.0f, 4096.0f, 0.0f }, { 0.0f, 0.0f, 4096.0f } };
  const float Tobj[3] = { (float)originX, (float)originY, (float)originZ };
  EObjXform cam;
  projComposeObjectHost(Robj, Tobj, &cam);

  // STEP 5/6 — the LCG. Each axis takes the ARITHMETIC high half of the state. Particle 0 reads the
  // RAW seed before any step; there is then one extra step whose value is never read, because the
  // guest's software pipeline steps again before its first read. Reproduced exactly: getting this
  // sequence wrong shifts the whole pattern.
  const uint32_t mul = c->mem_r32(kDotLcgMulA);
  uint32_t s = n.lcgSeed();
  auto axis = [&s]() { return (int32_t)s >> 16; };
  auto step = [&s, mul]() { s = s * mul + 1u; };

  int px = axis(); step();
  int py = axis(); step();
  int pz = axis(); step();          // this third step's value is deliberately never read

  RenderQueue& rq = c->game->activeRq();
  ObjScope objScope(c, node);
  int drawn = 0;

  for (int i = 0; i < kDotCount; i++) {
    const int vx = (px + dx) & (kDotLattice - 1);
    const int vy = (py + dy) & (kDotLattice - 1);
    const int vz = (pz + dz) & (kDotLattice - 1);

    ProjVtx pv;
    cam.project((int16_t)vx, (int16_t)vy, (int16_t)vz, &pv);

    // GATE 1 is the guest's GTE FLAG test (overflow / saturation anywhere in the transform). The
    // meaningful case for a float producer is a point at or behind the near plane; the rest of the
    // flag's bits are saturation states this path cannot reach.
    // GATE 2 is the guest's own screen clip, read UNSIGNED so a negative X fails: 0 <= SX < 320.
    // There is NO Y test — that asymmetry is the guest's, and it is kept.
    if (pv.sz > 0) {
      const int sx = (int)pv.px, sy = (int)pv.py;
      if (sx >= 0 && sx < 320) {
        const int sz = (pv.sz < kDotNearSz) ? 2 : 1;   // the only depth cue the effect has
        const float ord = proj_pz_to_ord(pv.pz);
        const int xs[4] = { sx, sx + sz, sx, sx + sz };
        const int ys[4] = { sy, sy, sy + sz, sy + sz };
        const float xsf[4] = { (float)xs[0], (float)xs[1], (float)xs[2], (float)xs[3] };
        const float ysf[4] = { (float)ys[0], (float)ys[1], (float)ys[2], (float)ys[3] };
        const float depth[4] = { ord, ord, ord, ord };
        const unsigned char w[4] = { 255, 255, 255, 255 };   // GP0 0x60 monochrome 0xFFFFFF, OPAQUE
        const int uv[4] = { 0, 0, 0, 0 };
        rq.emitOrQueue(c, /*capture=*/1, RQ_WORLD, RQ_OM_DEPTH, /*nv=*/4, /*semi=*/0, /*raw=*/0,
                       xs, ys, xsf, ysf, uv, uv, w, w, w, depth,
                       kDotMode, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1023, 511, 0);
        drawn++;
      }
    }

    step(); px = axis();
    step(); py = axis();
    step(); pz = axis();
  }

  if (cfg_dbg("fxdot"))
    cfg_logf("fxdot", "dotfield node=%08X origin=(%d,%d,%d) d=(%d,%d,%d) drawn=%d/%d",
             node, originX, originY, originZ, dx, dy, dz, drawn, kDotCount);
}
