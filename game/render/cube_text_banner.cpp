// game/render/cube_text_banner.cpp — see cube_text_banner.h for WHY this exists (it replaces two
// deleted taps) and for the rule it satisfies.
//
// RE SOURCE (2026-08-04, Ghidra headless + generated/ shards as ground truth; the full write-path
// audit is in docs/findings/render.md "cube-text banner — the transform is view space"):
//
//   The per-glyph record IS the sub-part. textLabelEmit's `cmd` and subPartWalk's `sub` are the same
//   pointer, node+0xC0[i], so ONE transform drives both halves of a character — its plank and its
//   letter. That is why drawing both from one computed transform here makes kanban #64's glyph-vs-
//   plank drift structurally impossible rather than merely small.
//
//   Exactly one function writes that matrix: NodeXform::propagateRotmat (FUN_80051300, already
//   native), once per tick from GraphicsBind::renderUpdateBody (FUN_800517F8, already native):
//       node.R  = rotmat(node.euler @node+0x54)          -> node+0x98
//       node.T  = (s16 node+0x2E, s16 node+0x32, s16 node+0x36)   -> node+0xAC/0xB0/0xB4
//       rec.R   = node.R * rotmat(rec.euler @rec+0x08)   -> rec+0x18
//       rec.T   = node.R * (s16 rec+0, rec+2, rec+4) + node.T     -> rec+0x2C
//   Nothing composes a camera into it, before or after. 19 functions on the write path were audited
//   for absolute reads of the scene camera (0x1F8000F8 / 0x1F80010C / 0x1F800118) and for
//   gte_read_ctrl: ZERO of either. The only scratchpad address any of them touches is 0x1F800000,
//   propagateRotmat's temp matrix. So this node class is a view-space billboard nailed to the
//   frustum, which the live measurement had already shown (camera eye travelled 383 units while the
//   glyph translations stayed exactly (-114, -64+-3, 358) / (-54, ..., 358)).
//
//   Z = node+0x36 is H+8, where H is the projection-plane distance at 0x801003F8 (350, or 233 in
//   area 3 — Pool::finalViewInit). It is read live below; 358 is NOT a constant of this effect.
//
// WHY THE TRANSFORM IS RECOMPUTED HERE RATHER THAN READ FROM rec+0x18. Reading rec+0x18 back would
// be reading a matrix the engine composed — the thing that got banned — and it is also strictly
// LESS information: the s16 CR packing has already thrown away precision this producer can keep in
// float. Recomputing from the five s16 fields the (already native) behaviour owns is reading the
// effect's own game state, which is exactly what a native producer is supposed to do.
//
// WHAT IS DELIBERATELY NOT DONE HERE: the animation is NOT re-simulated. beh_cube_text_spawn and its
// leaves already own it and already advance rec+0/+2/+8 and node+0x36 in guest memory each logic
// frame, and re-simulating would have to draw from the shared global LCG at 0x80105EE8 (2 draws per
// glyph at toss-in, 1 at bob-in, 1 at scatter-out) and desynchronise every other consumer of it.
// This producer reads the result. It also does NOT interpolate at 60fps: the toss-in ends with a
// one-frame snap (rec+0 -> exact layout X, rec+8 -> 0) and the Z pull-in ends with a clamp, so a
// naive lerp across a phase boundary would smear the spin. The banner steps at the logic rate, which
// is what its state actually says — the same stance BbRec particles already take.
#include "cube_text_banner.h"
#include "core.h"
#include "fps60.h"
#include "game.h"
#include "game_ctx.h"
#include "producer_scope.h" // ProducerScope — graphics-producer DB, native leg
#include "proj_params.h"    // ProjParams::pzToOrd
#include "render.h"
#include "render_internal.h" // rend()
#include <cmath>
#include <lucent/log.h>

namespace {

// ---- node / record layout (offsets named for what they ARE, per the RE above) --------------------
constexpr uint32_t NODE_BEHAVIOUR = 0x1Cu;   // u32 — the behaviour fn ptr; the class identity
constexpr uint32_t NODE_VARIANT = 0x03u;     // u8  — 2 = the "Clear" banner (different clut)
constexpr uint32_t NODE_GLYPH_COUNT = 0x08u; // u8  — L; node+9 is the do/while bound, both == L
constexpr uint32_t NODE_LOOP_BOUND = 0x09u;
constexpr uint32_t NODE_POS_X = 0x2Eu;      // s16 — 0 for this class, read anyway
constexpr uint32_t NODE_POS_Y = 0x32u;      // s16 — -64
constexpr uint32_t NODE_POS_Z = 0x36u;      // s16 — the rise-in: 2H+16 stepping down to H+8
constexpr uint32_t NODE_EULER = 0x54u;      // 3x s16 (x @+0x54, y @+0x56, z @+0x58) — all 0
constexpr uint32_t NODE_CMD_ARRAY = 0xC0u;  // u32[L] — the per-glyph records
constexpr uint32_t NODE_STRING_IDX = 0x60u; // s16 — index into the cube-text string table

constexpr uint32_t REC_POS_X = 0x00u;   // s16 — layout X (+ toss/scatter animation)
constexpr uint32_t REC_POS_Y = 0x02u;   // s16 — bob / toss Y
constexpr uint32_t REC_POS_Z = 0x04u;   // s16 — 0 at spawn, never written again
constexpr uint32_t REC_EULER = 0x08u;   // 3x s16 — X = the toss spin, Y/Z always 0
constexpr uint32_t REC_GEOMBLK = 0x40u; // u32 — the PLANK mesh (same block for every glyph)

// ---- string source (beh_cube_text_spawn STATE 0, reproduced: this is where the TEXT comes from) --
constexpr uint32_t STR_TABLE_ENTRY_PTR = 0x800A33CCu; // +node.s16[0x60]*12 -> char*
constexpr uint32_t CLEAR_STRING_PTR = 0x800A3A8Cu;    // the "Clear" variant's literal

// ---- glyph quad: the fixed template FUN_80039F4C builds at sp+16..47 every call -----------------
constexpr int16_t kGlyphX[4] = {-3, 5, -3, 5};
constexpr int16_t kGlyphY[4] = {-7, -7, 9, 9};
constexpr int16_t kGlyphZ = -1;

// ---- glyph material (FUN_80039F4C's post-projection patches + FUN_80039E80's atlas lookup) ------
constexpr uint32_t kGlyphOpByte = 0x2Du;  // GP0 FT4, textured RAW (bit0) — vertex colour ignored
constexpr uint32_t kGlyphRgb = 0x808080u; // neutral; raw texturing does not read it
constexpr uint32_t kGlyphTpage = 0x1Fu;
constexpr uint32_t kClutNormal = 0x7DFFu; // 32255
constexpr uint32_t kClutClear = 0x7C7Fu;  // 31871, the node+3==2 variant
constexpr uint8_t kSpace = 32u;           // FUN_80039E80 returns -1 for it: no glyph, but the
                                          // record still exists and still gets a plank.

// ---- plank geomblk layout (identical to the GT3/GT4 submitters in submit.cpp, ground truth) -----
//   header @+0: u32 counts -> low16 = #GT3 (36B records), high16 = #GT4 (44B records)
//   records @+16: nGT3 GT3 then nGT4 GT4, model-space s16 verts.
constexpr uint32_t COL_MASK = 0xFFF0F0F0u; // the GPU's per-byte low-nibble clear; keeps the op byte
constexpr int kMaxPrimsPerList = 4096;     // a block declaring more than this is not a geomblk

// The libgte angle unit: 4096 == one full turn. (Corroborated by the effect's own timing — the idle
// bob advances its phase by 128 per logic frame with a 32-frame period, and 32*128 == 4096.)
constexpr float kTurn = 4096.0f;

inline int16_t lo16(uint32_t v) {
  return (int16_t)(v & 0xFFFFu);
}
inline int16_t hi16(uint32_t v) {
  return (int16_t)(v >> 16);
}

// rotMatrix — libgte RotMatrix in float. Element formulas taken from Math::rotmat (gte_math.cpp,
// FUN_80085480) so the two agree; float instead of the s16 GPF chain because a native producer owns
// its own precision and has no byte-exactness contract. In practice only the X angle is ever nonzero
// for this class (the toss spin), and it is 0 for the whole settled life of the banner.
struct Mat3 {
  float m[3][3];
};
Mat3 rotMatrix(int16_t ax, int16_t ay, int16_t az) {
  const float rx = (float)ax * 6.28318530718f / kTurn;
  const float ry = (float)ay * 6.28318530718f / kTurn;
  const float rz = (float)az * 6.28318530718f / kTurn;
  const float sx = std::sin(rx), cx = std::cos(rx);
  const float sy = std::sin(ry), cy = std::cos(ry);
  const float sz = std::sin(rz), cz = std::cos(rz);
  Mat3 r;
  r.m[0][0] = cz * cy;
  r.m[0][1] = -sz * cy;
  r.m[0][2] = sy;
  r.m[1][0] = cz * sx * sy + cx * sz;
  r.m[1][1] = cx * cz - sz * sx * sy;
  r.m[1][2] = -cy * sx;
  r.m[2][0] = sx * sz - cz * cx * sy;
  r.m[2][1] = sx * cz + sz * cx * sy;
  r.m[2][2] = cy * cx;
  return r;
}
Mat3 matMul(const Mat3 &a, const Mat3 &b) { // a * b
  Mat3 o;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      o.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
    }
  }
  return o;
}

// One glyph's view-space transform, rebuilt from the banner's own state.
struct GlyphXform {
  Mat3 R;
  float T[3];
};

// ViewProjector — project a VIEW-SPACE point with the camera's projection constants and NOTHING
// ELSE. There is deliberately no camera rotation/translation member: this producer cannot be made
// camera-dependent by a later edit without adding one, which is the whole point of the class.
class ViewProjector {
public:
  ViewProjector(float ofx, float ofy, float h) : mOfx(ofx), mOfy(ofy), mH(h) {}
  bool valid() const {
    return mH > 0.0f;
  }
  // Returns false if the point is behind the eye (the caller drops the whole quad, matching the
  // guest's GTE-FLAG reject).
  bool project(const float v[3], float &sx, float &sy, float &ord, Core *c) const {
    if (v[2] <= 0.0f) {
      return false;
    }
    float pz = mH * 0.5f; // same near-plane clamp the world path uses
    if (v[2] > pz) {
      pz = v[2];
    }
    const float ph = mH / pz;
    sx = mOfx + v[0] * ph;
    sy = mOfy + v[1] * ph;
    ord = c->rsub.projParams.pzToOrd(v[2]);
    return true;
  }

private:
  float mOfx, mOfy, mH;
};

void transformCorner(const GlyphXform &g, float x, float y, float z, float out[3]) {
  for (int i = 0; i < 3; i++) {
    out[i] = g.R.m[i][0] * x + g.R.m[i][1] * y + g.R.m[i][2] * z + g.T[i];
  }
}

// Project + emit one quad of 4 model-space corners under a glyph's transform. Drops the quad if any
// corner is behind the eye.
void emitQuad(Core *c,
              uint32_t node,
              const ViewProjector &proj,
              const GlyphXform &g,
              const int16_t vx[4],
              const int16_t vy[4],
              const int16_t vz[4],
              const uint32_t col[4],
              uint32_t uv0,
              uint32_t uv1,
              uint32_t uv2,
              uint32_t uv3) {
  float px[4], py[4], dep[4];
  // One line per QUAD, not per corner (lucent::Line — accumulate, flush once). This is the line that
  // catches a producer projecting into nowhere: it carries the model corner, the view-space point and
  // the screen point together, so an off-screen result can be traced to the transform or the
  // projection without a second run. (It is how the double-draw at screen x -2328..2642 was found.)
  lucent::Line ln;
  for (int i = 0; i < 4; i++) {
    float view[3];
    transformCorner(g, (float)vx[i], (float)vy[i], (float)vz[i], view);
    if (!proj.project(view, px[i], py[i], dep[i], c)) {
      lucent::debug("cubetext", "quad DROPPED at corner {} (behind the eye): model=({},{},{})", i, vx[i], vy[i], vz[i]);
      return;
    }
    ln.add("[{} m=({},{},{}) v=({:.1f},{:.1f},{:.1f}) s=({:.1f},{:.1f})]",
           i,
           vx[i],
           vy[i],
           vz[i],
           view[0],
           view[1],
           view[2],
           px[i],
           py[i]);
  }
  ln.flush_debug("cubetext");
  Render::emitRecordQuad(c, node, col, uv0, uv1, uv2, uv3, px, py, dep);
}

// The PLANK half: decode the record's geomblk host-side and emit every prim under the same glyph
// transform. Same decode the native GT3/GT4 submitters use.
void emitPlanks(Core *c, uint32_t node, const ViewProjector &proj, const GlyphXform &g, uint32_t geomblk) {
  if (!geomblk) {
    return;
  }
  const uint32_t counts = c->mem_r32(geomblk);
  const int n3 = (int)(counts & 0xFFFFu), n4 = (int)((counts >> 16) & 0xFFFFu);
  if (n3 > kMaxPrimsPerList || n4 > kMaxPrimsPerList) {
    return; // not a geomblk
  }
  uint32_t rec = geomblk + 16;

  // GT3 textured tris (36B) — the 4th corner degenerates onto v2, the same split submit.cpp uses.
  for (int t = 0; t < n3; t++, rec += 36) {
    const uint32_t xy0 = c->mem_r32(rec + 16), xy1 = c->mem_r32(rec + 24), xy2 = c->mem_r32(rec + 28);
    const uint32_t vz01 = c->mem_r32(rec + 20), z2w = c->mem_r32(rec + 32);
    const int16_t vx[4] = {lo16(xy0), lo16(xy1), lo16(xy2), lo16(xy2)};
    const int16_t vy[4] = {hi16(xy0), hi16(xy1), hi16(xy2), hi16(xy2)};
    const int16_t vz[4] = {lo16(vz01), hi16(vz01), lo16(z2w), lo16(z2w)};
    const uint32_t code0 = c->mem_r32(rec + 0), rgb1w = c->mem_r32(rec + 4);
    const uint32_t rgb2 = (rgb1w << 4) & COL_MASK;
    const uint32_t col[4] = {code0 & COL_MASK, rgb1w & COL_MASK, rgb2, rgb2};
    const uint32_t uv0 = c->mem_r32(rec + 8), uv1 = c->mem_r32(rec + 12);
    const uint32_t uv2 = (uint32_t)(uint16_t)(z2w >> 16);
    emitQuad(c, node, proj, g, vx, vy, vz, col, uv0, uv1, uv2, uv2);
  }
  // GT4 textured quads (44B).
  for (int t = 0; t < n4; t++, rec += 44) {
    const uint32_t xy0 = c->mem_r32(rec + 20), xy1 = c->mem_r32(rec + 28), xy2 = c->mem_r32(rec + 32),
                   xy3 = c->mem_r32(rec + 40);
    const uint32_t vz01 = c->mem_r32(rec + 24), vz23 = c->mem_r32(rec + 36);
    const int16_t vx[4] = {lo16(xy0), lo16(xy1), lo16(xy2), lo16(xy3)};
    const int16_t vy[4] = {hi16(xy0), hi16(xy1), hi16(xy2), hi16(xy3)};
    const int16_t vz[4] = {lo16(vz01), hi16(vz01), lo16(vz23), hi16(vz23)};
    const uint32_t code0 = c->mem_r32(rec + 0), rgb2w = c->mem_r32(rec + 4);
    const uint32_t col[4] = {code0 & COL_MASK, (code0 << 4) & COL_MASK, rgb2w & COL_MASK, (rgb2w << 4) & COL_MASK};
    const uint32_t uv0 = c->mem_r32(rec + 8), uv1 = c->mem_r32(rec + 12), uv23 = c->mem_r32(rec + 16);
    emitQuad(c, node, proj, g, vx, vy, vz, col, uv0, uv1, (uint32_t)(uint16_t)uv23, (uint32_t)(uint16_t)(uv23 >> 16));
  }
}

// The GLYPH half. UVs are FUN_80039E80's atlas lookup, reproduced: u0 = (ch & 31) * 8,
// v0 = ((ch + 32) >> 5) * 16 + 8, corners (u0,v0) (u0+8,v0) (u0,v0+16) (u0+8,v0+16), with the
// guest's own wrap fixup — a u that lands on 0 after the +8 is written as 255 instead.
void emitGlyph(Core *c, uint32_t node, const ViewProjector &proj, const GlyphXform &g, uint8_t ch, uint32_t clut) {
  if (ch == kSpace) {
    return; // FUN_80039E80 returns -1: no glyph packet at all
  }
  const uint32_t u0 = (uint32_t)(ch & 31u) * 8u;
  const uint32_t v0 = ((uint32_t)(ch + 32u) >> 5) * 16u + 8u;
  uint32_t u1 = (u0 + 8u) & 0xFFu;
  if (u1 == 0u) {
    u1 = 255u; // the guest's wrap fixup, reproduced
  }
  const uint32_t v2 = (v0 + 16u) & 0xFFu;
  const uint32_t col = (kGlyphOpByte << 24) | kGlyphRgb;
  const uint32_t cols[4] = {col, col, col, col};
  emitQuad(c,
           node,
           proj,
           g,
           kGlyphX,
           kGlyphY,
           (const int16_t[4]){kGlyphZ, kGlyphZ, kGlyphZ, kGlyphZ},
           cols,
           u0 | (v0 << 8) | (clut << 16),
           u1 | (v0 << 8) | (kGlyphTpage << 16),
           u0 | (v2 << 8),
           u1 | (v2 << 8));
}

} // namespace

void CubeTextBanner::render(Core *c, uint32_t node) {
  if (!pictureBuildAllowed(c->game->oracle, fps60(*c->game).mWorldCaptureOnly)) {
    return;
  }
  if (c->mem_r32(node + NODE_BEHAVIOUR) != kBehCubeTextSpawn) {
    return;
  }

  // Producer DB, native leg — the LARGEST single undeclared block until now (68,388 prims in a 500-frame
  // replay). Keyed on guest FUN_80039F4C, which game/render/text_label.cpp INSTALLS at its own override
  // site, so the literal is the code's own assertion rather than a number copied from a comment.
  //
  // 0x80039F4C is the right key for the WHOLE producer, both halves: gen_func_80039F4C sets r5=1 and
  // calls func_8003F174 (the sub-part MESH pass = the PLANKS) before emitting one glyph quad per
  // character, so the planks are part of what it submits. Measured split of the 68,388: 8,340 glyph
  // quads (12.2%) and 60,048 plank prims (87.8%) — a key describing only the glyph half would misname
  // seven eighths of it. codemap's port-map step for this address is literally
  // `render-producer-cube-text-banner`.
  //
  // REJECTED, with reasons, because both look plausible: 0x8003F174 (subPartWalk) is the plank half's
  // SHARED writer and would collapse the type-1 sub-part class into this row; 0x800803DC is the generic
  // GT3/GT4 emitter this chain reaches (flag&1 forces it) and already carries 152,981 prims from
  // perObjFlush, so keying here would bury the banner inside an unidentified row.
  //
  // Scoped HERE and not at the render_walk.cpp call site: that site carries every other `pre` node type
  // too, all of which self-filter to zero prims, so a scope there would mint a row for a producer that
  // drew nothing.
  ProducerScope bannerScope(&c->rsub.producerScope, 0x80039F4Cu, "CubeTextBanner::render");

  const uint32_t count = c->mem_r8(node + NODE_GLYPH_COUNT);
  const uint32_t bound = c->mem_r8(node + NODE_LOOP_BOUND);
  if (count == 0 || bound == 0) {
    return;
  }
  const uint32_t glyphs = count < bound ? count : bound; // the guest breaks on BOTH counts

  // Projection constants only — ofx/ofy/H. Rcam/Tcam are read and DISCARDED on purpose: this class
  // is view-space, so applying the camera here is the bug that was just deleted. sceneCam is the
  // shared choke every native producer reads its projection through.
  float Rcam[3][3], Tcam[3], ofx, ofy, H;
  fps60(*c->game).sceneCam(c, Rcam, Tcam, ofx, ofy, H);
  const ViewProjector proj(ofx, ofy, H);
  if (!proj.valid()) {
    return;
  }

  // The node's own transform: R from its eulers (identically 0 for this class, computed anyway so a
  // future rotated banner is not silently wrong), T from its three position halfwords. node+0x36 is
  // the live rise-in Z; it is H+8 once settled, which is why the banner sits ~1:1 in screen units.
  const Mat3 nodeR = rotMatrix(
      c->mem_r16s(node + NODE_EULER + 0), c->mem_r16s(node + NODE_EULER + 2), c->mem_r16s(node + NODE_EULER + 4));
  const float nodeT[3] = {(float)c->mem_r16s(node + NODE_POS_X),
                          (float)c->mem_r16s(node + NODE_POS_Y),
                          (float)c->mem_r16s(node + NODE_POS_Z)};

  // The text. Same source beh_cube_text_spawn STATE 0 picks: the "Clear" literal for variant 2,
  // otherwise the cube-text string table entry the node was spawned with.
  const bool clearVariant = c->mem_r8(node + NODE_VARIANT) == 2u;
  const uint32_t clut = clearVariant ? kClutClear : kClutNormal;
  uint32_t textPtr = clearVariant
                         ? c->mem_r32(CLEAR_STRING_PTR)
                         : c->mem_r32(STR_TABLE_ENTRY_PTR + (uint32_t)(c->mem_r16s(node + NODE_STRING_IDX) * 3) * 4u);

  lucent::debug("cubetext",
                "node={:08X} glyphs={} nodeT=({},{},{}) ofx={} ofy={} H={} clear={}",
                node,
                glyphs,
                nodeT[0],
                nodeT[1],
                nodeT[2],
                ofx,
                ofy,
                H,
                clearVariant);

  bool textEnded = false;
  for (uint32_t i = 0; i < glyphs; i++) {
    const uint32_t rec = c->mem_r32(node + NODE_CMD_ARRAY + i * 4u);
    if (!rec) {
      continue;
    }

    // rec.R = node.R * rotmat(rec.euler);  rec.T = node.R * rec.pos + node.T.  Exactly
    // NodeXform::propagateRotmat, recomputed from the fields it reads rather than read back out of
    // the matrix it writes.
    GlyphXform g;
    g.R = matMul(nodeR,
                 rotMatrix(c->mem_r16s(rec + REC_EULER + 0),
                           c->mem_r16s(rec + REC_EULER + 2),
                           c->mem_r16s(rec + REC_EULER + 4)));
    const float lp[3] = {
        (float)c->mem_r16s(rec + REC_POS_X), (float)c->mem_r16s(rec + REC_POS_Y), (float)c->mem_r16s(rec + REC_POS_Z)};
    for (int k = 0; k < 3; k++) {
      g.T[k] = nodeR.m[k][0] * lp[0] + nodeR.m[k][1] * lp[1] + nodeR.m[k][2] * lp[2] + nodeT[k];
    }

    // Plank first, then the letter on top of it — one transform, so they cannot separate (#64).
    // The two halves have DIFFERENT bounds in the guest and must keep them: subPartWalk draws a plank
    // for every record (node+8/+9), while textLabelEmit's glyph loop additionally stops at the
    // string's NUL. Collapsing them into one `break` loses the trailing planks.
    emitPlanks(c, node, proj, g, c->mem_r32(rec + REC_GEOMBLK));
    if (textPtr && !textEnded) {
      const uint8_t ch = c->mem_r8(textPtr + i);
      if (ch == 0) {
        textEnded = true; // the guest's own end-of-string break, glyphs only
      } else {
        emitGlyph(c, node, proj, g, ch, clut);
      }
    }
  }
}
