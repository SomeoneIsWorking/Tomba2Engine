// game/render/fx_vortex.cpp — NATIVE PRODUCER for area 15's central PORTAL / VORTEX (kanban #44).
//
// The symptom: under pc_render area 15's swirling portal was simply absent (39325 px vs psx_render at the
// settled warp vantage). Cause, from guest data: `debug otattr` attributes the missing layer's packets to
// fn 0x80027A4C called from 0x801143C4 on node 0x800ED9E8 (behaviour 0x801140F4) — an A0F-OVERLAY custom
// render fn on a type-0x20 node. fieldObjectsRender's type-0x20 branch knew three emitter families and
// this is a fourth, so the node fell through and nothing drew. No producer, no picture.
//
// RE — Ghidra (scratch/decomp/vortex.c, from a live area-15 RAM dump) cross-read against the ground-truth
// gen body generated/ov_a0f_shard_0.c:ov_a0f_gen_801143C4. The emitter draws THREE layers:
//
//   (1) LENS FLARE (only while the behaviour's state byte node+4 == 1). RTPS the world anchor, then walk
//       the anchor→screen-centre line: three glow sprites at 7/8, 3/8 and 3/2 of the vector, half-extents
//       12 / 32 / 48 px, brightness (offscreenPenalty + rand&31 + 48/40), clut page index 6/6/7. Each is
//       written by FUN_80114288 as TWO stacked FT4s (upper half + v-mirrored lower half) at a fixed
//       near OT bucket, so it paints over the room. `offscreenPenalty` is the emitter's own visibility
//       fade: it sums how far off-screen the anchor is, and the whole flare is skipped below -50.
//
//   (2) The node's OWN 8-byte record list (node+0x34) drawn at the anchor, scaled by the behaviour's
//       node+0x50 / node+0x54 pair (MAC0·s·2 >> 8) with the depth cue driven to 1024 + rand&1023 toward
//       a BLACK far colour — the dark core of the portal. The behaviour collapses node+0x50 by 0x10 and
//       stretches node+0x54 by 0x80 per frame in its state 2, which is the portal closing.
//
//   (3) A SPHERE OF PARTICLES of radius (s16)node+0x4E: latitude θ = rcos(frame·64)>>5 + k·512 for the
//       eight k, longitude φ = frame·32 + j·step with step = 4096/(rsin θ >> 8) so arc spacing stays
//       even. Each point is anchor + (cosφ·sinθ·R, cosθ·R, sinφ·sinθ·R), projected on its own, drawn at
//       a quarter of its own MAC0 scale, cued by (4096 - node[0x50]·16) + rand&2047 toward a BLUE far
//       colour (0,0,1020) — which is why the swirl reads as dark blue-grey smoke.
//
// PORTED, NOT TAPPED: every position here comes from the node's own state projected through
// projComposeCamera (the fps60-lerped scene camera) — no gen body runs for the picture, no gte_op, no
// guest write. The one guest coupling the emitter has is the PRNG it consumes per particle; this producer
// READS the seed at Rng::SEED_ADDR and iterates a HOST copy, so the sparkle keeps the guest's character
// and stays identical between a real frame and its interpolated twin (the guest seed has not moved
// between the two presents) while never writing guest memory.
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include "render_internal.h"   // ObjScope / proj_pz_to_ord / proj_near_pz
#include "fx_sprite.h"         // SpriteAnchor
#include "game_ctx.h"          // trigOf(c) / rngOf(c)
#include "trig.h"              // Trig::rsin / rcos
#include "rng.h"               // Rng::SEED_ADDR
#include "fps60.h"             // Fps60::mObjOverrideOn — which present pass this is (diagnostic only)
#include "cfg.h"

namespace {

// --- node field layout (RE'd; live node 0x800ED9E8, behaviour 0x801140F4) --------------------------
constexpr uint32_t kState     = 0x04u;   // behaviour state byte (1 = open/idle, 2 = collapsing)
constexpr uint32_t kAnchorXY  = 0x2Cu;   // packed VX (lo16) | VY (hi16), world
constexpr uint32_t kAnchorZ   = 0x30u;   // VZ in lo16, world
constexpr uint32_t kOtBias    = 0x32u;   // s16 OT-key bias (-80 for this effect)
constexpr uint32_t kRecList   = 0x34u;   // the node's own 8-byte record list
constexpr uint32_t kClutPage  = 0x44u;   // clut (lo16) | tpage (hi16) for the core sprite
constexpr uint32_t kRadius    = 0x4Eu;   // s16 particle-sphere radius (0xF0 at spawn)
constexpr uint32_t kScaleX    = 0x50u;   // 8.8 X scale numerator (collapses by 0x10 per frame)
constexpr uint32_t kScaleY    = 0x54u;   // 8.8 Y scale numerator (grows by 0x80 per frame)

constexpr uint32_t kFrameCtr  = 0x1F80017Cu;   // the engine's u16 frame counter (drives the rotation)
constexpr uint32_t kPartRecs  = 0x801208B4u;   // the STATIC particle record list, in the A0F overlay
constexpr uint32_t kPartClut  = 0x801208B0u;   // its clut|tpage word (immediately before the list)
constexpr int      kDqa       = 6;             // the depth-cue constant the emitter programs

// The far colours the emitter loads before each layer: black for the core sprite, blue for the sphere.
constexpr int32_t kFarCore[3]  = { 0, 0, 0 };
constexpr int32_t kFarSphere[3] = { 0, 0, 1020 };

// Flare geometry, straight out of the three FUN_80114288 calls (fraction of the anchor→centre vector in
// eighths, half-extent in px, clut page index, brightness bias).
struct FlareGlow { int numer, denom, half, clutPage, lvlBias; };
constexpr FlareGlow kFlares[3] = { { 7, 8, 12, 6, 48 }, { 3, 8, 32, 6, 48 }, { 3, 2, 48, 7, 40 } };

// FUN_80114288's fixed material: one 4bpp texture page, and a clut row selected per glow.
constexpr uint16_t kFlareTpage = 0x35u;
constexpr int      kFlareU0 = 0xB9, kFlareU1 = 0xE7, kFlareV0 = 0x51, kFlareV1 = 0x67;

// The PSX libc LCG (guest FUN_8009A450) run on a HOST copy of the seed — read-only w.r.t. guest RAM.
class HostRng {
public:
  explicit HostRng(uint32_t seed) : mSeed(seed) {}
  uint32_t next() { mSeed = mSeed * 0x41C64E6Du + 0x3039u; return (mSeed >> 16) & 0x7FFFu; }
private:
  uint32_t mSeed;
};

}  // namespace

// FUN_801143C4 — area 15's portal render fn (A0F overlay), rebuilt natively.
void Render::a0fVortexRender(uint32_t node) {
  Core* c = mCore;

  // The scene camera, fps60-lerped at the interp present — the reason the whole portal interpolates.
  EObjXform cam; projComposeCamera(&cam);
  const uint32_t H = (uint32_t)cam.H;
  const int bias = (int16_t)c->mem_r16(node + kOtBias);
  const uint32_t axy = c->mem_r32(node + kAnchorXY);
  const uint32_t az  = c->mem_r32(node + kAnchorZ);
  const int anchorX = (int16_t)axy, anchorY = (int16_t)(axy >> 16), anchorZ = (int16_t)az;

  ProjVtx anchor;
  cam.project(anchorX, anchorY, anchorZ, &anchor);
  if (anchor.sz <= 0) return;                       // behind the camera: the emitter's FLAG check

  if (cfg_dbg("vortex"))
    cfg_logf("vortex", "present objov=%d anchor=(%.2f,%.2f) sz=%d node=%08X",
             c->game->fps60.mObjOverrideOn ? 1 : 0,
             (double)anchor.px, (double)anchor.py, anchor.sz, node);

  HostRng rng(c->mem_r32(Rng::SEED_ADDR));
  ObjScope objScope(c, node);   // prim identity (dbg_node) = the portal object

  // ---- (1) the lens flare, while the behaviour is in its open state -------------------------------
  // The emitter's own visibility fade: how far off-screen the anchor sits, in pixels, as a negative
  // number. Below -50 the flare is dropped entirely; otherwise it dims the glow.
  const int sx = anchor.sx, sy = anchor.sy;
  int penalty = 0;
  if (sx < 0)   penalty += sx;
  if (sy < 0)   penalty += sy;
  if (sx >= 320) penalty += 320 - sx;
  if (sy >= 240) penalty += 240 - sy;
  if (penalty < -50) return;                        // too far off-screen: the emitter drops ALL layers
  if (c->mem_r8(node + kState) == 1) {
    const float toCentreX = 160.0f - anchor.px, toCentreY = 120.0f - anchor.py;
    const int flicker = penalty + (int)(rng.next() & 31u);
    const float od = proj_pz_to_ord(proj_near_pz());   // the flare's fixed near OT bucket
    const float depth[4] = { od, od, od, od };
    for (const FlareGlow& f : kFlares) {
      const int lvl = flicker + f.lvlBias;
      if (lvl <= 0) continue;                          // FUN_80114288 draws nothing for a dead level
      const unsigned char g = (unsigned char)(lvl & 0xFF);
      unsigned char cc[4] = { g, g, g, g };
      const float cx = anchor.px + toCentreX * (float)f.numer / (float)f.denom;
      const float cy = anchor.py + toCentreY * (float)f.numer / (float)f.denom;
      const float h = (float)f.half;
      const uint16_t clut = (uint16_t)(((f.clutPage + 0x1F0) << 6) | 0x17);
      // Two stacked quads: the upper half, then the lower half with V mirrored (the guest's own split).
      const float px[4] = { cx - h, cx + h, cx - h, cx + h };
      const float pyTop[4] = { cy - h, cy - h, cy, cy };
      const float pyBot[4] = { cy, cy, cy + h, cy + h };
      const int us[4] = { kFlareU0, kFlareU1, kFlareU0, kFlareU1 };
      const int vTop[4] = { kFlareV0, kFlareV0, kFlareV1, kFlareV1 };
      const int vBot[4] = { kFlareV1, kFlareV1, kFlareV0, kFlareV0 };
      RenderQueue& rq = c->game->activeRq();
      rq.drawWorldQuad(c, px, pyTop, depth, us, vTop, cc, cc, cc, kFlareTpage, clut, /*semi=*/1, nullptr);
      rq.drawWorldQuad(c, px, pyBot, depth, us, vBot, cc, cc, cc, kFlareTpage, clut, /*semi=*/1, nullptr);
    }
  }

  // ---- (2) the portal core: the node's own record list at the anchor ------------------------------
  const uint32_t recList = c->mem_r32(node + kRecList);
  if (!recList) return;                              // no list -> the emitter emits nothing at all
  if (!SpriteAnchor::otKeyInRange(anchor.sz, bias)) return;   // the emitter's own emit/skip decision

  const int32_t scaleXn = (int32_t)c->mem_r32(node + kScaleX);
  const int32_t scaleYn = (int32_t)c->mem_r32(node + kScaleY);
  const int32_t anchorMac0 = SpriteAnchor::baseScale(H, anchor.sz, kDqa);
  const int32_t coreScaleX = (int32_t)(((int64_t)anchorMac0 * scaleXn) >> 7);
  const int32_t coreScaleY = (int32_t)(((int64_t)anchorMac0 * scaleYn) >> 7);
  const int32_t coreIr0 = 1024 + (int32_t)(rng.next() & 1023u);
  spriteRecordsEmit(recList, c->mem_r32(node + kClutPage), anchor.px, anchor.py,
                    proj_pz_to_ord(anchor.pz), coreScaleX, coreScaleY, coreIr0, kFarCore);

  // ---- (3) the rotating particle sphere ------------------------------------------------------------
  const int radius = (int16_t)c->mem_r16(node + kRadius);
  const uint32_t partClutPage = c->mem_r32(kPartClut);
  const int frame = (int)c->mem_r16(kFrameCtr);
  const int32_t sphereIr0Base = 4096 - (scaleXn << 4);
  const Trig& trig = trigOf(c);
  const int wobble = trig.rcos(frame * 64) >> 5;     // the whole sphere nods with the frame counter
  const int phi0   = frame << 5;                     // and spins

  for (int lat = 0; lat < 4096; lat += 512) {
    const int theta = wobble + lat;
    const int cosT = trig.rcos(theta), sinT = trig.rsin(theta);
    // Longitude step ∝ 1/sin θ so the arc spacing stays even; the polar rings collapse to one point.
    int step = 4096;
    if ((sinT >> 9) > 0) step = 4096 / (sinT >> 8);
    if (step >= 4097) continue;
    const int ringR = (int)(((int64_t)sinT * radius) >> 12);   // this latitude's ring radius
    const int ringY = (int)(((int64_t)cosT * radius) >> 12);
    for (int j = 0; ; ) {
      const int phi = phi0 + j;
      const int cosP = trig.rcos(phi), sinP = trig.rsin(phi);
      const int px16 = (int16_t)((uint16_t)anchorX + (uint16_t)(int)(((int64_t)cosP * ringR) >> 12));
      const int py16 = (int16_t)((uint16_t)anchorY + (uint16_t)ringY);
      const int pz16 = (int16_t)((uint16_t)anchorZ + (uint16_t)(int)(((int64_t)sinP * ringR) >> 12));

      ProjVtx pv;
      cam.project(px16, py16, pz16, &pv);
      if (SpriteAnchor::otKeyInRange(pv.sz, 0)) {
        const int32_t scale = SpriteAnchor::baseScale(H, pv.sz, kDqa) >> 2;
        const int32_t ir0 = sphereIr0Base + (int32_t)(rng.next() & 2047u);
        spriteRecordsEmit(kPartRecs, partClutPage, pv.px, pv.py, proj_pz_to_ord(pv.pz),
                          scale, scale, ir0, kFarSphere);
      }
      j += step;
      if (j + step >= 4097) break;
    }
  }
}
