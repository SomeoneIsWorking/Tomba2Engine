// class SwingFx — implementation. See swing_fx.h for the RE, the emitter contract, and why the
// scope around the shared emitter is mandatory rather than an optimisation.
#include "swing_fx.h"
#include "core.h"            // Core + gte_read_ctrl
#include "game.h"
#include "game_ctx.h"        // eng(c) / rend(c)
#include "engine.h"
#include "render.h"          // Render::projSetActive / projVertexActive + rsub.mode.psxRender() gate
#include "render_queue.h"    // RenderQueue::emitOrQueue + RQ_WORLD / RQ_OM_DEPTH
#include "projection.h"      // EObjXform / ProjVtx
#include "proj_params.h"     // proj_set_H / proj_pz_to_ord / proj_zsf4
#include "override_registry.h"
#include "cfg.h"             // `swingfx` diagnostic channel

extern void gen_func_8002A834(Core*);
extern void gen_func_80027768(Core*);

namespace {

// GTE control registers this producer READS as transform/colour inputs (never as results).
constexpr uint32_t kCrRot0    = 0u;    // CR0-4  — composed camera×object rotation, 1.3.12
constexpr uint32_t kCrTransX  = 5u;    // CR5-7  — composed view translation
constexpr uint32_t kCrFarR    = 21u;   // CR21-23 — far colour RFC/GFC/BFC (colour<<4)
constexpr uint32_t kCrOfx     = 24u;   // CR24/25 — screen centre, 16.16
constexpr uint32_t kCrH       = 26u;   // CR26   — projection-plane distance

// The depth-cue factor (GTE IR0) the caller publishes to scratchpad before the emitter runs.
constexpr uint32_t kScrDepthCue = 0x1F800090u;

// Face layout (see swing_fx.h). One face is 9 words / 36 bytes.
constexpr uint32_t kFaceStride  = 36u;
constexpr uint32_t kFaceMaxIter = 4096u;   // runaway guard — the guest's terminator is word1 bit31
constexpr uint32_t kSemiBit     = 0x40000000u;   // face word1 bit30 -> GP0 op 0x3E instead of 0x3C

// The OT bucket the guest quantizes its AVSZ4 depth into; a face whose bucket falls outside
// [4, 0x7FF] is never linked, i.e. never drawn. Mirrored so this producer culls exactly what the
// guest culls (gen_func_80027768: `k = (z >> (z>>10 & 31)) + ((z>>10) << 9)`, range-gated).
int32_t otBucket(int32_t depth) {
  const int32_t shift = depth >> 10;
  const int32_t k = (depth >> (shift & 31)) + (shift << 9);
  return ((uint32_t)(k - 4) >= 2044u) ? -1 : k;
}

// One channel of the guest's DPCS/DPCT depth-cue lerp toward the far colour (sf=1, lm=0):
//   MAC = c<<16 ; IR = clamp16(((FC<<12) - MAC) >> 12) ; MAC = (IR*IR0 + MAC) >> 12 ; out = MAC/16.
// At the charge effect's IR0 = 0xFFF this is ~the far colour; at IR0 = 0 it is the colour itself.
unsigned char depthCue(int colour, int32_t farColour, int32_t ir0) {
  const int32_t mac = colour << 16;
  int32_t ir = ((farColour << 12) - mac) >> 12;
  if (ir < -32768) ir = -32768; else if (ir > 32767) ir = 32767;
  int32_t out = ((ir * ir0 + mac) >> 12) / 16;
  if (out < 0) out = 0; else if (out > 255) out = 255;
  return (unsigned char)out;
}

}  // namespace

// Build the producer's transform from the composed GTE control registers the CALLER wrote, then walk
// the model's faces exactly as the guest emitter does and draw each survivor as a native world quad.
void SwingFx::drawMesh(uint32_t model, uint32_t clutBias, int32_t sortBias, uint8_t uBias) {
  Core* c = core;

  EObjXform xf;
  // CR0-4 halfword packing (same layout Render::projActiveCr writes): R00 R01 | R02 R10 | R11 R12 |
  // R20 R21 | R22.
  { const uint32_t g0 = gte_read_ctrl(kCrRot0 + 0), g1 = gte_read_ctrl(kCrRot0 + 1),
                   g2 = gte_read_ctrl(kCrRot0 + 2), g3 = gte_read_ctrl(kCrRot0 + 3),
                   g4 = gte_read_ctrl(kCrRot0 + 4);
    xf.R[0][0] = (int16_t)g0;        xf.R[0][1] = (int16_t)(g0 >> 16); xf.R[0][2] = (int16_t)g1;
    xf.R[1][0] = (int16_t)(g1 >> 16); xf.R[1][1] = (int16_t)g2;        xf.R[1][2] = (int16_t)(g2 >> 16);
    xf.R[2][0] = (int16_t)g3;        xf.R[2][1] = (int16_t)(g3 >> 16); xf.R[2][2] = (int16_t)g4; }
  for (int i = 0; i < 3; i++) xf.T[i] = (float)(int32_t)gte_read_ctrl(kCrTransX + (uint32_t)i);
  xf.ofx = (float)(int32_t)gte_read_ctrl(kCrOfx)     / 65536.0f;
  xf.ofy = (float)(int32_t)gte_read_ctrl(kCrOfx + 1) / 65536.0f;
  xf.H   = (float)(uint16_t)gte_read_ctrl(kCrH);
  if (xf.H <= 0.0f) return;                       // no camera projection published yet
  rend(c)->projSetActive(&xf);
  proj_set_H((uint16_t)xf.H);

  const int32_t ir0 = (int32_t)c->mem_r32(kScrDepthCue);
  const int32_t farColour[3] = { (int32_t)gte_read_ctrl(kCrFarR),
                                 (int32_t)gte_read_ctrl(kCrFarR + 1),
                                 (int32_t)gte_read_ctrl(kCrFarR + 2) };
  const int32_t zsf4 = proj_zsf4();
  const int drawOffX = c->game->gpu.s_off_x, drawOffY = c->game->gpu.s_off_y;
  int drawn = 0;

  uint32_t face = model;
  for (uint32_t n = 0; n < kFaceMaxIter; n++, face += kFaceStride) {
    const uint32_t w0 = c->mem_r32(face + 0), w1 = c->mem_r32(face + 4), w2 = c->mem_r32(face + 8);
    const bool last = (int32_t)w1 <= 0;           // word1 bit31 = terminator; that face is still drawn

    // Model-space corners: signed bytes * 256. X/Y sit in the tail bytes, Z in byte 3 of each of the
    // four colour words.
    auto sb = [&](uint32_t off) { return (int)(int8_t)c->mem_r8(face + off) * 256; };
    const int vx[4] = { sb(28), sb(29), sb(32), sb(33) };
    const int vy[4] = { sb(30), sb(31), sb(34), sb(35) };
    const int vz[4] = { sb(15), sb(19), sb(23), sb(27) };

    ProjVtx p[4];
    int xs[4], ys[4];
    float depth[4];
    int32_t szSum = 0;
    for (int k = 0; k < 4; k++) {
      rend(c)->projVertexActive(vx[k], vy[k], vz[k], &p[k]);
      xs[k] = p[k].sx + drawOffX; ys[k] = p[k].sy + drawOffY;
      depth[k] = proj_pz_to_ord(p[k].pz);
      szSum += p[k].sz;
    }
    // The guest's own visibility gate: AVSZ4 (ZSF4 * sum(SZ) >> 12) biased by the caller's sortBias,
    // quantized into an OT bucket; out-of-range = the face is never linked. No backface test exists
    // in this emitter (only GTE overflow flags), so neither is one added here.
    if (zsf4 > 0) {
      const int32_t avgSz = (int32_t)(((int64_t)zsf4 * szSum) >> 12);
      if (otBucket(avgSz + sortBias) < 0) { if (last) break; else continue; }
    }

    unsigned char r[4], g[4], b[4];
    for (int k = 0; k < 4; k++) {
      const uint32_t cw = c->mem_r32(face + 12u + (uint32_t)k * 4u);
      r[k] = depthCue((int)(cw & 0xFF),        farColour[0], ir0);
      g[k] = depthCue((int)((cw >> 8) & 0xFF), farColour[1], ir0);
      b[k] = depthCue((int)((cw >> 16) & 0xFF), farColour[2], ir0);
    }
    int u[4] = { (int)(uint8_t)((w0 & 0xFF) + uBias),        (int)(uint8_t)((w1 & 0xFF) + uBias),
                 (int)(uint8_t)((w2 & 0xFF) + uBias),        (int)(uint8_t)(((w2 >> 16) & 0xFF) + uBias) };
    int v[4] = { (int)((w0 >> 8) & 0xFF), (int)((w1 >> 8) & 0xFF),
                 (int)((w2 >> 8) & 0xFF), (int)((w2 >> 24) & 0xFF) };
    const uint16_t clut = (uint16_t)((w0 >> 16) + (clutBias << 6));   // clutBias<<22 in the uv0 word
    const uint16_t tp   = (uint16_t)((w1 >> 16) & 0x7F);
    const int semi = (w1 & kSemiBit) ? 1 : 0;
    const int tpX = (tp & 0xF) * 64, tpY = ((tp >> 4) & 1) * 256;
    const int mode = (tp >> 7) & 3, blend = (tp >> 5) & 3;
    const int clutX = (clut & 0x3F) * 16, clutY = (clut >> 6) & 0x1FF;

    // A GUEST-EXECUTION-TIME world prim (fps60.cpp isTier1Owned / the REDIRECT backlog): submitted with
    // resolved screen-space integers (xsf == nullptr -> has_xyf = 0) so BOTH the real and the
    // interpolated present replay it verbatim. drawWorldQuad's float path would set has_xyf = 1, which
    // marks a prim as OWNED BY the display-pass re-render — and tier1Render does not re-run this
    // effect, so the starburst was submitted and then skipped on both presents: 60 quads a frame and
    // not one pixel changed (measured, 2026-07-23). It still carries real per-vertex depth, so it
    // occludes against terrain and Tomba normally. It graduates to a display-pass producer when this
    // effect's transform joins the tier-1 re-render; until then it steps at the logic rate, exactly
    // like the rest of the guest-time world drawables (torch flame, AP gems, splash).
    c->game->activeRq().emitOrQueue(c, /*capture=*/1, RQ_WORLD, RQ_OM_DEPTH, /*nv=*/4, semi, /*raw=*/0,
                                    xs, ys, nullptr, nullptr, u, v, r, g, b, depth,
                                    mode, tpX, tpY, clutX, clutY,
                                    0, 0, 0, 0, 0, 0, 1023, 511, blend);
    drawn++;
    if (last) break;
  }
  rend(c)->projClearActive();
  if (cfg_dbg("swingfx")) { static long n = 0; if ((n++ & 63) == 0)
    cfg_logf("swingfx", "#%ld model=%08X faces=%d ir0=%d fc=(%d,%d,%d) sortBias=%d",
             n, model, drawn, ir0, farColour[0] >> 4, farColour[1] >> 4, farColour[2] >> 4, sortBias); }
}

// FUN_8002A834 — the charge-effect render fn. It owns no state this producer needs to change: the
// guest half is the untouched gen body, and the wrapper exists purely to scope the emitter tap.
void SwingFx::effectDrawTick(Core* c) {
  SwingFx& fx = eng(c).swingFx;
  const bool outer = !fx.mInEffectDraw;
  fx.mInEffectDraw = true;
  gen_func_8002A834(c);
  if (outer) fx.mInEffectDraw = false;
}

// FUN_80027768 — the shared packed-mesh quad emitter. Run the untouched guest body (packet pool / OT
// / pool cursor stay byte-exact), then re-derive this call's faces natively — but ONLY inside the
// effect scope, because this leaf is shared with ~16 other callers including the natively-produced
// field terrain.
void SwingFx::meshEmitTap(Core* c) {
  const uint32_t model    = c->r[4];
  const uint32_t clutBias = c->r[5];
  const int32_t  sortBias = (int32_t)(int16_t)c->r[6];
  const uint8_t  uBias    = (uint8_t)c->r[7];
  gen_func_80027768(c);
  if (c->game->oracle || c->rsub.mode.psxRender()) return;   // read-only overlay gate
  SwingFx& fx = eng(c).swingFx;
  if (!fx.mInEffectDraw || !model) return;
  fx.drawMesh(model, clutBias, sortBias, uBias);
}

void SwingFx::install() {
  engine_set_override_main(0x8002A834u, &SwingFx::effectDrawTick, gen_func_8002A834);
  // 0x80027768 is NOT installed here — game/render/mesh_emit_tap.cpp is its single owner and calls
  // SwingFx::drawMesh when mInEffectDraw is up. FxMesh (#15) needs the same leaf, and two installs on
  // one address silently drop one of them (the #28 failure mode).
}
