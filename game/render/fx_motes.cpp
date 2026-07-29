// game/render/fx_motes.cpp — the AREA-8 MOTION-STREAK MOTES, guest FUN_80116904 (A08 overlay).
// Found by the 22-area nofx sweep; per claim C012 it currently draws nothing.
//
// NOT a sprite-family member: it builds its primitives itself and never calls FUN_800329E0 /
// FUN_800317CC / FUN_80027A4C, so SpriteAnchor does not apply. In particular its OT-key gate looks
// like SpriteAnchor::otKeyInRange but is NOT the same function — see kGateNoPreClamp below.
//
// RE from ov_a08_gen_80116904, statically verified 2026-07-28 with its verifier's corrections
// (docs/re/render-targets-static-re.md — read the ORIGINAL algorithm alongside the CORRECTED delta).
//
// WHAT IT DRAWS: 32 world-space motes as gouraud semi-transparent LINES, each running from the mote's
// current screen position back to a point ONE FULL FRAME-DELTA BEYOND its previous position
// (tail = 2*prev - cur), white at the head fading to mid-grey at the tail — a doubled-length motion
// streak. Positions come from an LCG seeded at node+0x50 (never written back), masked to 11 bits so
// the field lives in a 2048-unit cube re-centred on the camera each frame; because each mote's world
// coordinate is (lcg + nodeAnchor) the motes hold WORLD positions and wrap, and a per-axis bit-11
// test against last frame's cube base suppresses the streak exactly on the frame a mote wraps, so no
// line ever whips across the volume.
//
// THE PREVIOUS FRAME COMES FROM A HOST SHADOW, not from the guest's array — see fx_motes.h for why.
//
// Read-only: no guest write anywhere, and notably node+0x50 is read and never written back, matching
// the guest.
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include "render_internal.h"   // ObjScope / proj_pz_to_ord
#include "projection.h"        // EObjXform
#include "fx_node.h"          // FxNode — the walk-owned header lens this controller extends
#include "fx_motes.h"
#include "cfg.h"
#include <cstdint>

const MoteFrame* MoteStreaks::submit(Core* c, uint32_t node, const MoteFrame& live) {
  const int frame = c->game->gpu.s_frame;
  if (mFrame != frame) { mPrev.swap(mCur); mCur.clear(); mFrame = frame; }
  mCur[node] = live;
  auto p = mPrev.find(node);
  return (p == mPrev.end()) ? nullptr : &p->second;
}

namespace {

// The A08 mote controller's OWN view of its node. Same reason as DotFieldNode: +0x2C is three
// separate s16 here, +0x50 is an LCG seed, and +0x48 is a per-frame cube base that no other member
// of the family carries at that offset.
class MoteNode : public FxNode {
public:
  using FxNode::FxNode;
  int32_t  anchorX()   const { return s16(0x2Cu); }
  int32_t  anchorY()   const { return s16(0x2Eu); }
  int32_t  anchorZ()   const { return s16(0x30u); }
  int32_t  prevBaseX() const { return s16(0x48u); }   // last frame's cube base — the wrap reference
  int32_t  prevBaseY() const { return s16(0x4Au); }
  int32_t  prevBaseZ() const { return s16(0x4Cu); }
  uint32_t lcgSeed()   const { return u32(0x50u); }   // READ ONLY
};

constexpr uint32_t kMoteLcgMul   = 0x801450D8u;  // A08 overlay data: the LCG multiplier
constexpr uint32_t kCamViewRow3X = 0x1F800104u;  // third row of the scene view rotation = forward axis
constexpr uint32_t kCamEyeX      = 0x1F8000D2u;  // camera world position X, then Y at +4, Z at +8
constexpr int      kLattice      = 2048;
constexpr int      kHalfCube     = 1024;
constexpr int      kWrapBit      = 2048;         // bit 11 — the wrap test
constexpr int      kLineMode     = 3;            // untextured gouraud
constexpr int      kLineBlend    = 3;

// The emitter's OT-key gate. It is the family's logarithmic bucket map with bias 0 but WITHOUT the
// `if (k < 4) k = 4` pre-clamp SpriteAnchor::otKeyInRange applies — so this emitter REJECTS the near
// range that otKeyInRange would accept. Reusing otKeyInRange here would silently draw motes the guest
// drops.
inline bool kGateNoPreClamp(int sz) {
  const uint32_t z = (uint32_t)(sz & 0xFFFF);
  const uint32_t e = z >> 12;
  const int32_t key = (int32_t)((z >> 2) >> (e & 31)) + (int32_t)(e << 9);
  return (uint32_t)(key - 4) < 2044u;
}

}  // namespace

void Render::fxMoteStreakRender(uint32_t node) {
  Core* c = mCore;

  // The wrap cube's centre: half a cube behind the camera eye, nudged by half the camera forward row.
  const int h0 = c->mem_r16s(kCamViewRow3X) >> 1;
  const int h1 = c->mem_r16s(kCamViewRow3X + 2u) >> 1;
  const int h2 = c->mem_r16s(kCamViewRow3X + 4u) >> 1;
  const int offsX = (int16_t)(uint16_t)(c->mem_r16(kCamEyeX)      - kHalfCube + h0);
  const int offsY = (int16_t)(uint16_t)(c->mem_r16(kCamEyeX + 4u) - kHalfCube + h1);
  const int offsZ = (int16_t)(uint16_t)(c->mem_r16(kCamEyeX + 8u) - kHalfCube + h2);

  const MoteNode n(c, node);
  const int baseX = n.anchorX() - offsX;
  const int baseY = n.anchorY() - offsY;
  const int baseZ = n.anchorZ() - offsZ;
  const int prevBaseX = n.prevBaseX();
  const int prevBaseY = n.prevBaseY();
  const int prevBaseZ = n.prevBaseZ();

  // camR . (V + offs) + camT — the same shape the area-11 dot field uses.
  const float Robj[3][3] = { { 4096.0f, 0.0f, 0.0f }, { 0.0f, 4096.0f, 0.0f }, { 0.0f, 0.0f, 4096.0f } };
  const float Tobj[3] = { (float)offsX, (float)offsY, (float)offsZ };
  EObjXform cam;
  projComposeObjectHost(Robj, Tobj, &cam);

  const uint32_t mul = c->mem_r32(kMoteLcgMul);
  uint32_t s = n.lcgSeed();

  MoteFrame live;
  int wrapped = 0;
  struct Head { float x, y; bool ok; } head[MoteFrame::kMotes];

  for (int i = 0; i < MoteFrame::kMotes; i++) {
    // Three steps per mote, each axis reading the seed BEFORE its replacement.
    const int gx = (int32_t)s >> 16; s = s * mul + 1u;
    const int gy = (int32_t)s >> 16; s = s * mul + 1u;
    const int gz = (int32_t)s >> 16; s = s * mul + 1u;

    const int ax = gx + baseX, ay = gy + baseY, az = gz + baseZ;   // unmasked, for the wrap test
    ProjVtx pv;
    cam.project((int16_t)(ax & (kLattice - 1)), (int16_t)(ay & (kLattice - 1)),
                (int16_t)(az & (kLattice - 1)), &pv);

    head[i].ok = false;
    if (pv.sz <= 0 || !kGateNoPreClamp(pv.sz)) continue;

    // The wrap gate: did this mote's 11-bit coordinate roll over since last frame? If so it teleported
    // across the cube and must not be streaked.
    if (((ax ^ (gx + prevBaseX)) & kWrapBit) || ((ay ^ (gy + prevBaseY)) & kWrapBit) ||
        ((az ^ (gz + prevBaseZ)) & kWrapBit)) { wrapped++; continue; }

    head[i].x = pv.px; head[i].y = pv.py; head[i].ok = true;
    live.sx[i] = pv.px; live.sy[i] = pv.py; live.valid[i] = true;
  }

  const MoteFrame* prev = mMoteStreaks.submit(c, node, live);

  RenderQueue& rq = c->game->activeRq();
  ObjScope objScope(c, node);
  int drawn = 0;
  if (prev) {
    for (int i = 0; i < MoteFrame::kMotes; i++) {
      if (!head[i].ok || !prev->valid[i]) continue;      // the guest's sentinel case: seed history only
      // tail = 2*prev - cur: one full frame-delta beyond the previous position.
      const float tx = head[i].x + 2.0f * (prev->sx[i] - head[i].x);
      const float ty = head[i].y + 2.0f * (prev->sy[i] - head[i].y);

      const float dx = tx - head[i].x, dy = ty - head[i].y;
      const float len = dx * dx + dy * dy;
      if (len < 1e-4f) continue;                          // zero-length: the GPU would draw nothing
      const float inv = 0.5f / __builtin_sqrtf(len);
      const float nx = -dy * inv, ny = dx * inv;          // half of a 1px stroke

      const float xsf[4] = { head[i].x + nx, tx + nx, head[i].x - nx, tx - nx };
      const float ysf[4] = { head[i].y + ny, ty + ny, head[i].y - ny, ty - ny };
      int xs[4], ys[4];
      for (int k = 0; k < 4; k++) { xs[k] = (int)(xsf[k] + 0.5f); ys[k] = (int)(ysf[k] + 0.5f); }
      const float depth[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
      // white at the head, mid-grey at the tail — the guest's 0xFFFFFF -> 0x808080 gouraud.
      const unsigned char col[4] = { 255, 128, 255, 128 };
      const int uv[4] = { 0, 0, 0, 0 };
      rq.emitOrQueue(c, /*capture=*/1, RQ_OVERLAY, RQ_OM_2D_FG, /*nv=*/4, /*semi=*/1, /*raw=*/0,
                     xs, ys, xsf, ysf, uv, uv, col, col, col, depth,
                     kLineMode, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1023, 511, kLineBlend);
      drawn++;
    }
  }

  if (cfg_dbg("fxmote"))
    cfg_logf("fxmote", "motes node=%08X base=(%d,%d,%d) wrapped=%d streaks=%d/%d prev=%s",
             node, baseX, baseY, baseZ, wrapped, drawn, MoteFrame::kMotes, prev ? "yes" : "no");
}
