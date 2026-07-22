// class FxMesh — implementation. See fx_mesh.h for the architecture note, the PRIMAT/otattr
// measurement that identified the missing layer, and why the tap has to be scoped.
#include "fx_mesh.h"
#include "core.h"
#include "game.h"
#include "game_ctx.h"          // eng(c)
#include "engine.h"            // Engine owns the FxMesh instance
#include "render.h"            // rsub.mode.psxRender() gate
#include "render_internal.h"   // ObjScope / cur_render_node / proj_pz_to_ord / render_queue.h
#include "projection.h"        // EObjXform::project — the float RTPT
#include "cfg.h"
#include "override_registry.h"

extern void gen_func_800288AC(Core*);
extern void gen_func_80027768(Core*);

namespace {

// ── The 9-word (36-byte) template record FUN_80027768 walks ──────────────────────────────────
// Traced from generated/shard_5.c gen_func_80027768 (the recompiler's own translation — ground
// truth for GTE-bearing code; Ghidra renders the COP2 traffic as unresolvable pseudo-calls).
//   +0   uv0 (low16) | CLUT id (high16)   — the clut-row argument lands at bit 22 of this word
//   +4   uv1 (low16) | texpage (high16)   — the guest masks the word with 0x7FFFFF, so only
//        texpage bits 0-6 survive (base X/Y + blend mode; colour depth is always 4bpp here).
//        bit 30 = SEMI-TRANSPARENT (GP0 op 0x3E rather than 0x3C); bit 31 = LAST RECORD.
//   +8   uv2 (low16) | uv3 (high16)
//   +12  rgb0 (bytes 0-2) | model Z0 (byte 3)      the Z coordinates ride in the colour words'
//   +16  rgb1             | model Z1               otherwise-unused top byte
//   +20  rgb2             | model Z2
//   +24  rgb3             | model Z3
//   +28  X0 X1 Y0 Y1   — one signed byte per coordinate; the model value is byte << 8
//   +32  X2 X3 Y2 Y3
constexpr uint32_t kRecStride   = 36u;
constexpr uint32_t kRecSemiBit  = 0x40000000u;   // word+4 bit30 -> GP0 op 0x3E (semi-transparent)
constexpr uint32_t kTpageMask   = 0x7Fu;         // what survives the guest's 0x7FFFFF word mask
constexpr uint32_t kClutArgShift = 22u;          // the clut-row argument's bit position in word+0

// The effect fade factor FUN_800288AC publishes for the guest's DPCT/DPCS depth-cue ops: 0 = full
// colour, 4096 = fully faded. It also zeroes the GTE far colour (CR21-23), so the depth cue reduces
// to a plain scale of the record's own RGB — see cueScale below.
constexpr uint32_t kDepthCueScratch = 0x1F800090u;
constexpr int32_t  kCueUnit         = 4096;

// GTE control registers the composed transform lives in when FUN_80027768 is entered.
constexpr unsigned kCrRot0 = 0, kCrTrans0 = 5, kCrOfx = 24, kCrOfy = 25, kCrH = 26, kCrZsf4 = 30;

// The game's own visible-depth range: a quad whose OT bucket index falls outside [4, 0x7FF] is
// never linked and therefore never drawn. Reproduced so this producer draws exactly the set of
// quads the game draws — it is the effect's near/far cull, expressed in the game's units.
constexpr int32_t kBucketMin = 4, kBucketSpan = 2044;

struct MeshRecord {
  Core*    c;
  uint32_t at;

  uint32_t uv0Clut()  const { return c->mem_r32(at + 0);  }
  uint32_t uv1Tpage() const { return c->mem_r32(at + 4);  }
  uint32_t uv23()     const { return c->mem_r32(at + 8);  }
  uint32_t colorZ(int i) const { return c->mem_r32(at + 12u + (uint32_t)i * 4u); }
  uint32_t cornerXY(int i) const { return c->mem_r32(at + (i < 2 ? 28u : 32u)); }

  bool semi()      const { return (uv1Tpage() & kRecSemiBit) != 0; }
  bool lastRecord() const { return (int32_t)uv1Tpage() <= 0; }   // guest loops while the word is > 0

  // Model-space corner i. X/Y are interleaved byte pairs; Z is the colour word's top byte.
  void corner(int i, int* x, int* y, int* z) const {
    const uint32_t xy = cornerXY(i);
    const int lane = i & 1;                       // corner 0/2 -> low byte, 1/3 -> high byte
    *x = (int8_t)(xy >> (lane * 8)) * 256;
    *y = (int8_t)(xy >> (16 + lane * 8)) * 256;
    *z = (int8_t)(colorZ(i) >> 24) * 256;
  }
};

// Depth cue with a zero far colour = a straight scale toward black. The effect-mesh controller
// always writes CR21-23 = 0 immediately before the call, so this is the whole of the guest's
// DPCT/DPCS effect on this path; anything else means the caller changed and this needs revisiting.
float cueScale(Core* c) {
  const int32_t cue = (int32_t)c->mem_r32(kDepthCueScratch);
  if (cue <= 0) return 1.0f;
  if (cue >= kCueUnit) return 0.0f;
  return (float)(kCueUnit - cue) / (float)kCueUnit;
}

// The composed transform the effect-mesh controller left in the GTE control registers (object
// scale/rotation folded with the scene camera). Read, never written — projected in float.
EObjXform composedXform(Core* c) {
  EObjXform x{};
  // CR0-4 pack the nine s16 rotation terms across five words, in libgte MATRIX order.
  const uint32_t r0 = gte_read_ctrl(kCrRot0 + 0), r1 = gte_read_ctrl(kCrRot0 + 1),
                 r2 = gte_read_ctrl(kCrRot0 + 2), r3 = gte_read_ctrl(kCrRot0 + 3),
                 r4 = gte_read_ctrl(kCrRot0 + 4);
  x.R[0][0] = (int16_t)r0;        x.R[0][1] = (int16_t)(r0 >> 16); x.R[0][2] = (int16_t)r1;
  x.R[1][0] = (int16_t)(r1 >> 16); x.R[1][1] = (int16_t)r2;        x.R[1][2] = (int16_t)(r2 >> 16);
  x.R[2][0] = (int16_t)r3;        x.R[2][1] = (int16_t)(r3 >> 16); x.R[2][2] = (int16_t)r4;
  for (int i = 0; i < 3; i++) x.T[i] = (float)(int32_t)gte_read_ctrl(kCrTrans0 + (unsigned)i);
  x.ofx = (float)(int32_t)gte_read_ctrl(kCrOfx) / 65536.0f;
  x.ofy = (float)(int32_t)gte_read_ctrl(kCrOfy) / 65536.0f;
  x.H   = (float)(uint16_t)gte_read_ctrl(kCrH);
  return x;
}

// The effect's AUTHORED DEPTH OFFSET, converted from the game's depth-key units into view units.
// The controller passes the effect's own node+0x32 as the sort bias (measured -80 for the impact
// burst): the game adds it to the quad's averaged depth before choosing where to draw it, which is
// how a burst that spawns just BEHIND the object it hit still paints in front of it. AVSZ4 forms
// that average as `(sum_of_4_depths * ZSF4) >> 12`, so one unit of bias is 4096/(4*ZSF4) units of
// view depth — the same scale submit.cpp's key_to_ord inverts. Without this the burst is a few
// percent farther than the bucket it hits and the depth buffer hides it (measured: 0.0892 ord vs
// the bucket's 0.0968).
float authoredDepthOffset(int32_t sortBias) {
  const int32_t zsf4 = (int16_t)gte_read_ctrl(kCrZsf4);
  if (zsf4 <= 0) return 0.0f;
  return (float)sortBias * 4096.0f / (4.0f * (float)zsf4);
}

// The game's own depth-bucket test for a quad: average the four view depths the way AVSZ4 does,
// add the caller's sort bias, run the guest's bucket compression, and report whether the result
// lands inside the linkable range. False = a quad the game drops, so the producer drops it too.
bool visibleDepth(Core* c, const ProjVtx p[4], int32_t sortBias) {
  const int32_t zsf4 = (int16_t)gte_read_ctrl(kCrZsf4);
  int64_t sum = 0;
  for (int i = 0; i < 4; i++) sum += (int32_t)p[i].sz;
  const int32_t avg = (int32_t)((sum * zsf4) >> 12);
  if (avg < 0) return false;
  const int32_t z = avg + sortBias;
  if (z < 0) return false;
  const int32_t shift  = z >> 10;
  const int32_t bucket = (z >> (shift & 31)) + (shift << 9);
  return (uint32_t)(bucket - kBucketMin) < (uint32_t)kBucketSpan;
}

}  // namespace

// FUN_80027768's re-derivation for THIS producer. The leaf itself is owned by the one tap in
// game/render/mesh_emit_tap.cpp, which runs the guest body, applies the read-only-overlay gate and
// then calls whichever producer's scope is up — SwingFx's (#14) or this one's. It used to be tapped
// here directly, which collided with SwingFx doing the same on the same address: two installs, second
// one silently wins, one effect never draws. See the banner in mesh_emit_tap.cpp.
// a0 = template list, a1 = clut-row selector, a2 = OT sort bias (s16), a3 = per-frame U scroll added
// to every corner's texture column.
void FxMesh::draw(Core* c, uint32_t list, uint32_t clutRow, int32_t sortBias, uint8_t uScroll) {
  if (!list) return;

  const EObjXform xf = composedXform(c);
  const float shade  = cueScale(c);
  const float depthOff = authoredDepthOffset(sortBias);
  ObjScope objScope(c, cur_render_node(c));                  // prim identity = the emitting object
  RenderQueue& rq = c->game->activeRq();
  const int ox = c->game->gpu.s_off_x, oy = c->game->gpu.s_off_y;   // GPU drawing-area offset

  if (cfg_dbg("fxmesh")) cfg_logf("fxmesh", "list=%08X clutRow=%u bias=%d uScroll=%u cue=%d",
                                  list, clutRow, sortBias, uScroll, (int32_t)c->mem_r32(kDepthCueScratch));
  MeshRecord rec{ c, list };
  for (int guard = 0; guard < 256; guard++, rec.at += kRecStride) {
    const bool last = rec.lastRecord();

    ProjVtx p[4];
    for (int i = 0; i < 4; i++) {
      int mx, my, mz; rec.corner(i, &mx, &my, &mz);
      xf.project(mx, my, mz, &p[i]);
    }

    if (visibleDepth(c, p, sortBias)) {
      const uint32_t uv0 = rec.uv0Clut(), uv1 = rec.uv1Tpage(), uv23 = rec.uv23();
      const uint16_t clut  = (uint16_t)((uv0 + (clutRow << kClutArgShift)) >> 16);
      const uint16_t tpage = (uint16_t)((uv1 >> 16) & kTpageMask);
      const int semi = rec.semi() ? 1 : 0;

      int xs[4], ys[4], u[4], v[4]; float depth[4]; uint8_t r[4], g[4], b[4];
      const uint32_t uvWord[4] = { uv0, uv1, uv23, uv23 >> 16 };
      for (int i = 0; i < 4; i++) {
        xs[i] = p[i].sx + ox; ys[i] = p[i].sy + oy;
        depth[i] = proj_pz_to_ord(p[i].pz + depthOff);
        u[i] = (uint8_t)(uvWord[i] + uScroll);
        v[i] = (uint8_t)(uvWord[i] >> 8);
        const uint32_t col = rec.colorZ(i);
        r[i] = (uint8_t)((float)(col & 0xFFu) * shade);
        g[i] = (uint8_t)((float)((col >> 8) & 0xFFu) * shade);
        b[i] = (uint8_t)((float)((col >> 16) & 0xFFu) * shade);
      }
      // GUEST-EXECUTION-TIME world prim, so INTEGER screen coords and no xsf/ysf: `has_xyf` is what
      // tells fps60 "the display pass re-renders this one" (runtime/recomp/fps60.cpp isTier1Owned).
      // This producer runs inside the guest's own render walk, not in the display pass, so claiming
      // it would make every interpolated frame drop these quads — measured: with drawWorldQuad's
      // float path the burst was submitted every frame and NOT ONE PIXEL of the picture changed.
      // Same contract as fx_sprite.cpp and perobj_billboard.cpp; it lifts when the emitter moves to
      // the display pass (docs/fps60-rework.md REDIRECT doctrine).
      rq.emitOrQueue(c, /*capture=*/1, RQ_WORLD, RQ_OM_DEPTH, 4, semi, /*raw=*/0,
                     xs, ys, nullptr, nullptr, u, v, r, g, b, depth,
                     (tpage >> 7) & 3, (tpage & 0xF) * 64, ((tpage >> 4) & 1) * 256,
                     (clut & 0x3F) * 16, (clut >> 6) & 0x1FF,
                     0, 0, 0, 0, 0, 0, 1023, 511, (tpage >> 5) & 3);
      if (cfg_dbg("fxmesh")) {
        cfg_logf("fxmesh", "quad rec=%08X semi=%d tp=%04X clut=%04X xy0=(%d,%d) xy3=(%d,%d) shade=%.3f rgb0=%02X%02X%02X uv0=(%d,%d) d=%.4f",
                 rec.at, semi, tpage, clut, xs[0], ys[0], xs[3], ys[3],
                 (double)shade, r[0], g[0], b[0], u[0], v[0], (double)depth[0]);
      }
    }
    if (last) break;                 // the guest emits the terminal record, then stops
  }
}

namespace {

// FUN_800288AC — the effect-mesh CONTROLLER: composes the effect's transform from its own world
// state and calls the shared writer. It owns no host state, so the guest half is the untouched
// gen body; the wrapper exists only to scope the writer's tap to this caller.
void armTap(Core* c) {
  FxMesh& fx = eng(c).fxMesh;
  fx.mScope++;
  gen_func_800288AC(c);
  fx.mScope--;
}

}  // namespace

void FxMesh::install() {
  engine_set_override_main(0x800288ACu, armTap, gen_func_800288AC);
  // 0x80027768 is NOT installed here — game/render/mesh_emit_tap.cpp is its single owner and calls
  // FxMesh::draw when this producer's scope is up. Installing it here too is what collided with #14.
}
