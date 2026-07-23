// game/render/fx_sprite.cpp — FxSprite: NATIVE PRODUCER for the FUN_80027A4C world-anchored
// scaled-sprite effect family (seaside torch / hut-roof flames — kanban #12 restored the layer, #23
// makes it LERP at fps60).
//
// WAS a leaf tap on 0x80027A4C (run the gen body, scrape the scratchpad the emitter published, re-derive
// the quads host-side). A tap fires at GUEST time on the real frame only and re-uses the guest's own
// RTPS result, so its output can never interpolate — the interp present re-run would have to re-execute
// the gen body under a lerped camera, which writes guest RAM. That is exactly #23: the roof flames step
// at 30 Hz. The fix (per CLAUDE.md "LERP IS NATIVE TOO" / "NATIVE PRESENTATION") is a real port: PROJECT
// the effect's own world state with the native camera and emit from the display pass, so tier1Render
// re-renders it one frame behind under the lerped camera like every other native producer.
//
// RE (Ghidra scratch/decomp/fx_emit_leaves.c + fx_pkt_writer.c, ground truth generated/shard_5..7.c). The
// family has ONE packet writer (0x80027A4C) and THREE emitter render-fns; a type-0x20 render node carries
// its emitter at node+0x18 (confirmed live at the seaside water-pump vista, HEAD 0x800F2624: six
// FUN_80027CB4 roof-flame nodes + eight FUN_800281EC particle-flame nodes, all vis=1). Every emitter:
//   1. loads the pure scene-camera CRs (scratchpad 0x1F8000F8..0x114) into the GTE and sets DQA=6 (or 4
//      for the FUN_800281EC '!' variant) / DQB=0 — i.e. it REPURPOSES the GTE's depth-cue divide as a
//      per-Z sprite scale: after RTPS, MAC0 (GTE data reg 0x18) = n·DQA with n=(H·0x20000/SZ3 + 1)/2.
//   2. RTPS the WORLD anchor (node+0x2C packed VX,VY / node+0x30 VZ): SXY2 = screen anchor, SZ3 = depth,
//      MAC0 = base pixel scale.
//   3. range-checks the OT bucket key quantize((SZ3>>2)+node+0x32); out of range -> skip (no emit).
//   4. FUN_80027A4C(a0 = 8-byte record list node+0x34, a1 = clut|tpage node+0x44) walks the records.
// Scale per emitter:
//   FUN_80027CB4  scaleX = MAC0                                (uniform)
//   FUN_80027E5C  scaleX = MAC0 * (u8)node+0x6  >> 4
//   FUN_800281EC  PER-PARTICLE: particles at node+0x50 stride 8, count (s16)node+0x4E; each = anchor
//                 (p[0] packed VX,VY / p[1] VZ) RTPS'd individually, scaleX = MAC0 * (s16)p+0x6 >> 8.
// The native camera projection (projComposeCamera -> EObjXform::project) reproduces the GTE RTPS in float
// (docs tomba2-native-projection, 0-diff vs Beetle) and — crucially — reads the camera through the
// Fps60::sceneCam choke, which returns the LERPED camera at the interp present. So projecting the anchor
// natively gives a screen position that MOVES SMOOTHLY as the camera pans: the flame lerps for free. The
// base scale is derived from the same native SZ3 + the emitter's own DQA constant (no ambient GTE read),
// so it too tracks the lerped depth. The 8-byte record walk (corners = anchor + s8-offset·scale>>16, UV
// flip rules, flat-grey brightness) is unchanged from the tap — only the anchor/scale SOURCE changed from
// "scrape the gen body's scratchpad" to "project the world anchor natively".
//
// 0x80027A4C now runs its plain gen body for guest-state fidelity (SBS stays byte-exact — the guest still
// executes it; pc_render simply no longer scrapes it). Read-only overlay: no guest write.
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include "render_internal.h"   // ObjScope / cur_render_node / proj_pz_to_ord
#include "cfg.h"

namespace {

// --- node field layout (RE'd; confirmed live) ------------------------------------------------------
constexpr uint32_t kAnchorXY = 0x2Cu;   // packed VX (lo16) | VY (hi16), world
constexpr uint32_t kAnchorZ  = 0x30u;   // VZ in lo16, world
constexpr uint32_t kOtBias   = 0x32u;   // s16 OT-key bias
constexpr uint32_t kRecList  = 0x34u;   // 8-byte record list (a0 to the packet writer)
constexpr uint32_t kClutPage = 0x44u;   // clut (lo16) | tpage (hi16) (a1)
constexpr uint32_t kRenderFn = 0x18u;   // custom render fn = the emitter address
constexpr uint32_t kE5cScale = 0x06u;   // u8 extra scale numerator (FUN_80027E5C)
constexpr uint32_t kVariant3 = 0x03u;   // '!' selects DQA=4 in the FUN_800281EC variant
constexpr uint32_t kPartCount= 0x4Eu;   // s16 particle count (FUN_800281EC)
constexpr uint32_t kPartArray= 0x50u;   // particle array base, stride 8 (FUN_800281EC)
constexpr uint32_t kPartScale= 0x06u;   // s16 per-particle scale numerator (particle+6)

constexpr uint32_t FN_UNIFORM   = 0x80027CB4u;   // scaleX = MAC0
constexpr uint32_t FN_BYTESCALE = 0x80027E5Cu;   // scaleX = MAC0 * node[6] >> 4
constexpr uint32_t FN_PARTICLE  = 0x800281ECu;   // per-particle loop

// The emitters set DQA to a small integer and DQB to 0 before RTPS, so the depth-cue MAC0 becomes a
// per-Z sprite scale. baseScale reproduces MAC0 = n·DQA, n=(H·0x20000/SZ3 + 1)/2 (the RTPS perspective
// divide, integer-faithful; the GTE's UNR reciprocal differs by <1 ULP and never changes a pixel here).
int32_t baseScale(uint32_t H, int sz, int dqa) {
  int64_t div = ((int64_t)H << 17) / sz;          // H * 0x20000 / SZ3
  if (div > 0x1FFFF) div = 0x1FFFF;               // GTE divide saturates (also the SZ3-too-small case)
  const int64_t n = (div + 1) >> 1;
  return (int32_t)(n * dqa);
}

// The emitter's OT-key range gate (its emit/skip decision), reproduced on the native SZ3 so the producer
// draws exactly the anchors the guest would. Logarithmic bucket map, valid range [4, 0x7FF].
bool otKeyInRange(int sz, int bias) {
  if (sz <= 0) return false;
  int k = (sz >> 2) + bias;
  if (k < 4) k = 4;
  k = (k >> ((k >> 10) & 0x1f)) + (k >> 10) * 0x200;
  return (uint32_t)(k - 4) <= 0x7FBu;
}

// Walk the 8-byte quad records at rec0 and draw each as a textured world quad, sized by (scaleX,scaleY)
// about the native-projected float anchor (anchorXf,anchorYf) at view depth `od`. Identical record math
// to the former tap; corners are kept in float (sub-pixel) so the quad interpolates smoothly, and the
// draw goes through RenderQueue::drawWorldQuad (has_xyf=1 -> tier1-owned -> re-rendered under the lerped
// camera). Read-only.
void emitSpriteRecords(Core* c, uint32_t rec0, uint32_t clutPage,
                       float anchorXf, float anchorYf, float od, int32_t scaleX, int32_t scaleY) {
  const float sxf = (float)scaleX / 65536.0f, syf = (float)scaleY / 65536.0f;
  const float depth[4] = { od, od, od, od };
  uint32_t rec = rec0;
  for (int guard = 0; guard < 256; guard++) {
    const uint32_t w0 = c->mem_r32(rec);
    const uint32_t w1 = c->mem_r32(rec + 4u);
    const uint32_t flags = w1 >> 16;

    // Corners: float anchor + signed-byte offset * scale (the guest's (s8*scale)>>16, kept in float).
    const float xL = anchorXf + (float)(int8_t)(w0)       * sxf;
    const float xR = anchorXf + (float)(int8_t)(w0 >> 8)  * sxf;
    const float yT = anchorYf + (float)(int8_t)(w0 >> 16) * syf;
    const float yB = anchorYf + (float)(int8_t)(w0 >> 24) * syf;

    // UVs with the guest's flip rules (note the asymmetric 7/6 insets — mirrored exactly).
    const uint32_t u = w1 & 0xFFu, v = (w1 >> 8) & 0xFFu;
    const int ub = (int)(u & 0xF8u), uw = (int)(u & 7u) * 8;
    const int vb = (int)(v & 0xF8u), vh = (int)(v & 7u) * 8;
    int u02, u13, v01, v23;
    if (!(flags & 0x2000u)) { u02 = ub + 1;      u13 = ub + uw + 7; }
    else                    { u02 = ub + uw + 6; u13 = ub + 1;      }
    if (!(flags & 0x1000u)) { v01 = vb + 1;      v23 = vb + vh + 7; }
    else                    { v01 = vb + vh + 7; v23 = vb + 1;      }

    // Flat grey brightness (all RE'd callers force the DPCS depth-cue to 0, so the color passes through).
    const unsigned char col = (unsigned char)(flags & 0xFFu);
    const int semi = (int)(flags & 1u);
    const uint16_t clut  = (uint16_t)((clutPage & 0xFFFFu) + ((flags & 0xF00u) >> 2));
    const uint16_t tpage = (uint16_t)((clutPage >> 16) | ((flags & 6u) << 4));

    float px[4] = { xL, xR, xL, xR };
    float py[4] = { yT, yT, yB, yB };
    int   us[4] = { u02, u13, u02, u13 };
    int   vs[4] = { v01, v01, v23, v23 };
    unsigned char cc[4] = { col, col, col, col };
    c->game->activeRq().drawWorldQuad(c, px, py, depth, us, vs, cc, cc, cc, tpage, clut, semi, nullptr);

    rec += 8u;
    if (flags & 0xC000u) break;   // the terminal record IS drawn (gen's post-condition), then stop
  }
}

}  // namespace

void Render::fxSpriteRender(uint32_t node) {
  Core* c = mCore;
  const uint32_t rec0 = c->mem_r32(node + kRecList);
  if (!rec0) return;                                        // no record list -> the emitter emits nothing
  const uint32_t rfn      = c->mem_r32(node + kRenderFn);
  const uint32_t clutPage = c->mem_r32(node + kClutPage);
  const int bias          = (int16_t)c->mem_r16(node + kOtBias);

  // The scene camera, fps60-lerped at the interp present (the whole reason this producer interpolates).
  EObjXform cam; projComposeCamera(&cam);
  const uint32_t H = (uint32_t)cam.H;

  ObjScope objScope(c, node);   // prim identity (dbg_node) = the emitting flame object

  auto projectEmit = [&](int vx, int vy, int vz, int dqa, int32_t numer, int shift) {
    ProjVtx pv; cam.project(vx, vy, vz, &pv);
    if (!otKeyInRange(pv.sz, bias)) return;                 // same emit/skip gate the emitter applies
    int32_t scale = baseScale(H, pv.sz, dqa);
    if (shift) scale = (int32_t)(((int64_t)scale * numer) >> shift);   // E5C / per-particle modulation
    emitSpriteRecords(c, rec0, clutPage, pv.px, pv.py, proj_pz_to_ord(pv.pz), scale, scale);
  };

  if (rfn == FN_PARTICLE) {
    const int dqa   = (c->mem_r8(node + kVariant3) == '!') ? 4 : 6;
    const int count = (int16_t)c->mem_r16(node + kPartCount);
    for (int i = 0; i < count; i++) {
      const uint32_t p   = node + kPartArray + (uint32_t)i * 8u;
      const uint32_t axy = c->mem_r32(p);
      const uint32_t az  = c->mem_r32(p + 4u);
      const int16_t  mod = (int16_t)c->mem_r16(p + kPartScale);
      projectEmit((int16_t)axy, (int16_t)(axy >> 16), (int16_t)az, dqa, mod, 8);
    }
    return;
  }

  // FN_UNIFORM / FN_BYTESCALE: one world anchor.
  const uint32_t axy = c->mem_r32(node + kAnchorXY);
  const uint32_t az  = c->mem_r32(node + kAnchorZ);
  const int numer = (rfn == FN_BYTESCALE) ? (int)(uint8_t)c->mem_r8(node + kE5cScale) : 0;
  const int shift = (rfn == FN_BYTESCALE) ? 4 : 0;
  projectEmit((int16_t)axy, (int16_t)(axy >> 16), (int16_t)az, /*dqa=*/6, numer, shift);
}
