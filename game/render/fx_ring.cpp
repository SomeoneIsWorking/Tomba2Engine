// game/render/fx_ring.cpp — NATIVE PRODUCER for the IMPACT / SHOCK ANNULUS (guest FUN_8002ECD8 +
// its shared drawing leaf FUN_8002E680).
//
// THE GAP. 0x8002ECD8 is a type-0x20 node render fn (its pointer lives in the effect descriptor table
// at 0x800A222C, alongside 0x80027CB4 / 0x800281EC / 0x8010BF54 — all render fns this walk already
// owns). It was on no whitelist, so pc_render drew nothing for it at all: the flat ring that flashes
// out of an impact was simply absent from the picture. This is the class of gap the user pointed at
// with "Tomba's weapon attack impact (hitting something) missing".
//
// WHY IT IS TWO FUNCTIONS. FUN_8002E680 is a SHARED leaf — eleven call sites across MAIN.EXE and the
// A01/A06/A08/A0J overlays reach it. It knows nothing about nodes: it takes an inner radius, an outer
// radius and two colours, and reads its centre + scale from the scratchpad block its caller published.
// FUN_8002ECD8 is the node-facing half that fills that block in and animates the radii. Porting the
// pair together is what makes the leaf reusable for the other ten callers later.
//
// ============================ RE — FUN_8002E680, the annulus ============================
// Inputs: a0 = inner radius, a1 = outer radius, a2 = inner colour, a3 = outer colour.
// Scratchpad contract published by the caller:
//   0x1F800080  u32  OT bucket key            0x1F800084/0x1F800088  u32  pixel scale (always equal)
//   0x1F80008C  s16  centre X                 0x1F80008E             s16  centre Y
// Every emitted vertex is  centre + radius * scale>>16 * {cos,sin}(angle) — pure SCREEN space, no GTE
// and no per-vertex projection. The leaf is a 2D ring rasteriser.
//
// The vertex generator, settled with tools/mips_trace.py after two wrong hand-readings (see the
// instrument ledger, I017 — the store operands are compiler-rotated temporaries that cannot be read
// from a 3-instruction window):
//   * FIVE authored angles, 0x66 then the s16 table at 0x800A20A8 = {204, 307, 409, 512}. In the PSX
//     12-bit circle (4096 == 2*pi) those are 9, 18, 27, 36 and 45 DEGREES.
//   * Each iteration joins the PREVIOUS angle to the CURRENT one as one annulus segment: a gouraud
//     quad (GP0 0x3A) whose v0/v2 sit on the INNER radius carrying a2 and whose v1/v3 sit on the OUTER
//     radius carrying a3 — i.e. a RADIAL colour gradient across the ring's thickness. The caller
//     passes a2 = a3>>1 & 0x7F7F7F, so the gradient runs half-brightness -> full.
//   * Iteration 0's "previous" is the current angle MIRRORED (cos kept, sin negated), so the first
//     segment spans -9..+9 degrees about the axis.
//   * Every segment is emitted as its EIGHT DIHEDRAL IMAGES — (x,y), (y,x), (-x,y), (-y,-x), (x,-y),
//     (y,-x), (-x,-y), (-y,x) — which tiles the 0..45 degree wedge over the whole circle. Iteration 0
//     emits only the first FOUR, because its segment is already symmetric about the axis and the other
//     four images would be exact duplicates. 4 + 4*8 = 36 quads, which is the packet count the guest's
//     pool advance (0x144 words = 36 * 9) confirms.
// The leaf finishes by linking a DR_MODE prim (SetDrawMode tpage 0x35) in FRONT of the quads in the OT,
// so the whole ring draws with semi-transparency mode 1 = B+F: ADDITIVE. That is the blend used here.
//
// ============================ RE — FUN_8002ECD8, the node half ============================
// Two centre/scale modes, chosen by node+3:
//   node+3 == 0x91  — HUD variant: centre fixed at screen (32,32), scale 1.0, OT bucket 4 (frontmost).
//   otherwise       — WORLD variant: RTPS the node's own anchor (node+0x2C packed VX,VY / node+0x30 VZ)
//                     through the scene camera with DQA=6, DQB=0, so MAC0 is the family's usual
//                     depth-cue-as-pixel-scale (SpriteAnchor::baseScale). SXY2 is the centre, and the
//                     emit/skip decision is the family's OT-key range gate on SZ3 (otKeyInRange).
// Then the RADII, animated by the single byte node+5 stepped through the 12-bit circle in units of 64:
//   base  = 48 + 32*sin(a)          outer = ceil(base/2)
//   inner = ceil((base - 32*cos(a)) / 2)
// so the ring's thickness opens from (8..24) at a=0, closes to zero at a=90 degrees and inverts past
// it — one animator driving radius and thickness together. The s16 flag at 0x800E7FFE, when negative,
// halves both radii again (the guest's small-ring variant).
//
// PORTED, NOT TAPPED. Nothing here runs a guest-visible behavior; the centre and scale come from the NATIVE camera
// (projComposeCamera -> Fps60::sceneCam), so the ring interpolates at 60fps like the rest of this
// render tier, and the radii stay in float instead of being truncated to the guest's integer pixels.
// Read-only: guest memory is only read.
#include "cfg.h"
#include "core.h"
#include "fx_sprite.h" // SpriteAnchor — the shared baseScale / otKeyInRange relations
#include "game.h"
#include "game_ctx.h" // trigOf() — the per-Core Trig instance
#include "render.h"
#include "render_internal.h" // ObjScope / proj_pz_to_ord
#include "render_queue.h"
#include "trig.h" // class Trig — rsin/rcos over the guest LUT

namespace {

// --- the node's fields ----------------------------------------------------------------------------
constexpr uint32_t kVariantByte = 0x03u; // 0x91 selects the fixed-screen HUD ring
constexpr uint8_t kHudVariant = 0x91u;
constexpr uint32_t kAnimByte = 0x05u; // the single animator: angle = byte << 6
constexpr uint32_t kAnchorXY = 0x2Cu; // world anchor, packed VX (low) / VY (high)
constexpr uint32_t kAnchorZ = 0x30u;
constexpr uint32_t kRingColour = 0x64u; // the outer (full-brightness) colour; inner = >>1 & 0x7F7F7F

// --- the leaf's own constants ---------------------------------------------------------------------
constexpr int kDqa = 6;                           // depth-cue-as-scale numerator the node half programs
constexpr uint32_t kAngleTable = 0x800A20A8u;     // s16 x 4: the 2nd..5th authored angles
constexpr int32_t kFirstAngle = 0x66;             // 9 degrees — the 1st is an immediate, not a table entry
constexpr int kSegments = 5;                      // 5 authored angles -> 5 annulus segments per octant
constexpr int kRadiusBase = 0x30;                 // the 48 in base = 48 + 32*sin(a)
constexpr int kRadiusSwing = 5;                   // the 32: applied as (trig << 5) >> 12
constexpr uint32_t kHalfFlag = 0x800E7FFEu;       // s16 < 0 -> halve both radii again
constexpr int kHudCentreX = 32, kHudCentreY = 32; // the 0x200020 the HUD branch publishes
constexpr int kRingBlend = 1;                     // SetDrawMode tpage 0x35 -> ABR 1 = B+F, additive

// The eight dihedral images of a wedge point, in the guest's own emission order. Each entry says which
// source component feeds screen X and Y and with what sign — reading {sx, useYForX, sy, useXForY}
// as a small table is the honest shape of what the guest writes as eight unrolled store blocks.
struct Image {
  int xs, xsrc, ys, ysrc;
}; // src 0 = the cos component, 1 = the sin component
constexpr Image kImages[8] = {
    {+1, 0, +1, 1}, // ( x,  y)
    {+1, 1, +1, 0}, // ( y,  x)
    {-1, 0, +1, 1}, // (-x,  y)
    {-1, 1, -1, 0}, // (-y, -x)
    {+1, 0, -1, 1}, // ( x, -y)
    {+1, 1, -1, 0}, // ( y, -x)
    {-1, 0, -1, 1}, // (-x, -y)
    {-1, 1, +1, 0}, // (-y,  x)
};
// Iteration 0's segment is symmetric about the axis, so its second four images duplicate the first four
// exactly and the guest does not emit them.
constexpr int kImagesFirstSegment = 4;

// The guest's `n - ((s16)n >> 1)` halving, which rounds toward +inf for the positive radii it is used on.
inline int halfUp(int n) {
  return n - ((int)(int16_t)n >> 1);
}

} // namespace

// FUN_8002E680 — the shared annulus leaf, as a native producer. Centre and scale are already resolved
// (the caller's scratchpad contract, resolved natively); radii are in the guest's pre-scale units.
void Render::impactAnnulusDraw(float cx,
                               float cy,
                               float ord,
                               float scale,
                               int innerR,
                               int outerR,
                               uint32_t colInner,
                               uint32_t colOuter,
                               int layer,
                               int orderMode) {
  Core *c = mCore;
  const float ri = (float)innerR * scale, ro = (float)outerR * scale;
  if (ri == 0.0f && ro == 0.0f) {
    return; // a collapsed ring covers no pixels
  }

  const unsigned char ir = (unsigned char)(colInner & 0xFF), ig = (unsigned char)((colInner >> 8) & 0xFF),
                      ib = (unsigned char)((colInner >> 16) & 0xFF);
  const unsigned char orr = (unsigned char)(colOuter & 0xFF), og = (unsigned char)((colOuter >> 8) & 0xFF),
                      ob = (unsigned char)((colOuter >> 16) & 0xFF);
  // v0/v2 ride the inner radius, v1/v3 the outer — the radial gradient across the ring's thickness.
  const unsigned char rs[4] = {ir, orr, ir, orr};
  const unsigned char gs[4] = {ig, og, ig, og};
  const unsigned char bs[4] = {ib, ob, ib, ob};
  const float depth[4] = {ord, ord, ord, ord};
  const int uv[4] = {0, 0, 0, 0};

  RenderQueue &rq = c->game->activeRq();
  const Trig &tg = trigOf(c);

  int32_t prevAngle = -kFirstAngle; // segment 0 spans the mirrored pair (-9, +9) degrees
  for (int s = 0; s < kSegments; s++) {
    const int32_t curAngle = s == 0 ? kFirstAngle : (int32_t)(int16_t)c->mem_r16(kAngleTable + (uint32_t)(s - 1) * 2u);
    // The wedge's four corners, as (cos, sin) components on each radius. src index 0 = cos, 1 = sin.
    const float pc = (float)tg.rcos(prevAngle) / 4096.0f, ps = (float)tg.rsin(prevAngle) / 4096.0f;
    const float cc = (float)tg.rcos(curAngle) / 4096.0f, cs = (float)tg.rsin(curAngle) / 4096.0f;
    const float comp[4][2] = {{ri * pc, ri * ps},  // v0 — inner, previous angle
                              {ro * pc, ro * ps},  // v1 — outer, previous angle
                              {ri * cc, ri * cs},  // v2 — inner, current angle
                              {ro * cc, ro * cs}}; // v3 — outer, current angle

    const int nImages = s == 0 ? kImagesFirstSegment : 8;
    for (int im = 0; im < nImages; im++) {
      const Image &g = kImages[im];
      float xsf[4], ysf[4];
      int xs[4], ys[4];
      for (int v = 0; v < 4; v++) {
        xsf[v] = cx + (float)g.xs * comp[v][g.xsrc];
        ysf[v] = cy + (float)g.ys * comp[v][g.ysrc];
        xs[v] = (int)(xsf[v] < 0 ? xsf[v] - 0.5f : xsf[v] + 0.5f);
        ys[v] = (int)(ysf[v] < 0 ? ysf[v] - 0.5f : ysf[v] + 0.5f);
      }
      rq.emitOrQueue(c,
                     /*capture=*/1,
                     layer,
                     orderMode,
                     /*nv=*/4,
                     /*semi=*/1,
                     /*raw=*/0,
                     xs,
                     ys,
                     xsf,
                     ysf,
                     uv,
                     uv,
                     rs,
                     gs,
                     bs,
                     depth,
                     /*mode=*/3,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     1023,
                     511,
                     kRingBlend);
    }
    prevAngle = curAngle;
  }
}

// FUN_8002ECD8 — the node half: resolve centre/scale/depth, animate the radii, hand off to the leaf.
void Render::impactRingRender(uint32_t node) {
  Core *c = mCore;

  const uint32_t colOuter = c->mem_r32(node + kRingColour) & 0xFFFFFFu;
  const uint32_t colInner = (colOuter >> 1) & 0x7F7F7Fu;

  float cx, cy, ord, scale;
  int layer, orderMode;
  if (c->mem_r8(node + kVariantByte) == kHudVariant) {
    // The HUD ring: a fixed screen position with no perspective at all (the guest publishes scale
    // 0x10000, i.e. 1.0, and OT bucket 4 — in front of everything).
    cx = (float)kHudCentreX;
    cy = (float)kHudCentreY;
    scale = 1.0f;
    ord = 0.0f;
    layer = RQ_HUD;
    orderMode = RQ_OM_2D_FG;
  } else {
    EObjXform cam;
    projComposeCamera(&cam);
    const uint32_t axy = c->mem_r32(node + kAnchorXY);
    const uint32_t az = c->mem_r32(node + kAnchorZ);
    ProjVtx pv;
    cam.project((int16_t)axy, (int16_t)(axy >> 16), (int16_t)az, &pv);
    // The emitter's own reject: behind the camera, or an OT key outside [4, 0x7FF] -> it emits nothing.
    if (!SpriteAnchor::otKeyInRange(pv.sz, /*bias=*/0)) {
      return;
    }
    cx = pv.px;
    cy = pv.py;
    scale = (float)SpriteAnchor::baseScale((uint32_t)cam.H, pv.sz, kDqa) / 65536.0f;
    ord = proj_pz_to_ord(pv.pz);
    layer = RQ_WORLD;
    orderMode = RQ_OM_DEPTH;
  }

  // The radii, from the one animator byte. base opens the ring outward while the cos term closes its
  // thickness, so a single counter both grows the ring and thins it as it fades.
  const Trig &tg = trigOf(c);
  const int32_t a = (int32_t)(uint32_t)c->mem_r8(node + kAnimByte) << 6;
  const int32_t base = ((tg.rsin(a) << kRadiusSwing) >> 12) + kRadiusBase;
  int32_t outerR = halfUp(base);
  int32_t innerR = halfUp(base - ((tg.rcos(a) << kRadiusSwing) >> 12));
  if ((int16_t)c->mem_r16(kHalfFlag) < 0) { // the guest's small-ring variant: halve again
    outerR = (int32_t)((int16_t)outerR >> 1);
    innerR = (int32_t)((int16_t)innerR >> 1);
  }

  ObjScope objScope(c, node);
  impactAnnulusDraw(cx, cy, ord, scale, innerR, outerR, colInner, colOuter, layer, orderMode);

  if (cfg_dbg("fxring")) {
    cfg_logf("fxring",
             "node=%08X anim=%d centre=(%.1f,%.1f) scale=%.4f r=%d..%d col=%06X",
             node,
             (int)c->mem_r8(node + kAnimByte),
             (double)cx,
             (double)cy,
             (double)scale,
             innerR,
             outerR,
             colOuter);
  }
}
