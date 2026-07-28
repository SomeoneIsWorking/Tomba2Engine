// game/render/subpart_capture.cpp — Render::subPartCapture, the DISPLAY-PASS half of subPartWalk.
//
// WHY THIS EXISTS (kanban #64, and #16/#23 which are the same bug through different strings).
// A text-label node is drawn in two halves by Render::textLabelEmit (FUN_80039F4C):
//   - the MESH pass — func_8003F174 = Render::subPartWalk — which is what draws the WOODEN PLANKS
//     the letters sit on, and
//   - the GLYPH pass — one quad per character, each captured as a Render::WqRec and emitted by
//     Render::billboardsRender through the float camera path.
// Only the glyph half produced a display-pass record. subPartWalk is a faithful substrate mirror:
// it loads each sub-part's transform into the GTE and submits its geomblk through the still-
// substrate func_8003F698, which yields guest packets — and pc_render does not walk the guest OT.
// So the two halves sat on different presentation tiers: at 60fps the LETTERS interpolated under
// the lerped camera while the BOARDS stepped at 30Hz, and the glyphs visibly drifted off their
// planks letter-by-letter. That is the artefact in the user's "A Red Treasure Chest" capture.
//
// WHAT THIS DOES. For each sub-part, decode its geomblk host-side and push one WqRec per prim,
// using the SAME contract the glyph pass uses (model-space corners + the sub-part's transform
// factored against the scene camera). billboardsRender then projects and interpolates board and
// letters through one path, keyed the same way.
//
// This is a NATIVE PRODUCER, not a tag: identity is structural (the sub-part it came from), the
// picture is re-derived from the object's own geometry and transform, and nothing is stamped onto
// or matched against a guest packet. See CLAIM/directive "NATIVE PRESENTATION — no stamping".
//
// STRICTLY READ-ONLY. Guest memory is only read; no c->r[] register is touched (wq_read_matrix and
// wq_factor_world are pure mem_r32 reads), so subPartWalk's byte-exact guest behaviour — including
// its LIVE-REGISTER LAW spills, see subpart_walk.cpp — is unaffected. Skipped on the oracle leg.
//
// GEOMBLK LAYOUT (same as mesh_draw.cpp / submit.cpp's GT3/GT4 submitters, which are ground truth):
//   header @geomblk+0: u32 counts -> low16 = #GT3 (36B records), high16 = #GT4 (44B records)
//   records @geomblk+16: nGT3 GT3 records, then nGT4 GT4 records; model-space s16 verts.
//   GT3 (36B): +0 rgb0|op  +4 rgb1  +8 uv0|clut  +12 uv1|tpage  +16 XY0  +20 Z0|Z1
//              +24 XY1  +28 XY2  +32 Z2(lo)|uv2(hi)          rgb2 = rgb1<<4
//   GT4 (44B): +0 rgb0|op  +4 rgb2  +8 uv0|clut  +12 uv1|tpage  +16 uv2(lo)|uv3(hi)
//              +20 XY0  +24 Z0|Z1  +28 XY1  +32 XY2  +36 Z2|Z3  +40 XY3
//              rgb1 = rgb0<<4, rgb3 = rgb2<<4
#include "core.h"
#include "cfg.h"               // cfg_dbg / cfg_logf — `PSXPORT_DEBUG=subpartcap`
#include "game.h"
#include "game_ctx.h"
#include "render.h"
#include "render_internal.h"   // rend() / wq_read_matrix / wq_factor_world
#include <stdint.h>

namespace {

// The GPU's per-byte low-nibble clear on RGB889 colour words, exactly as the GT3/GT4 submitters
// apply it (submit.cpp COL_MASK). The TOP byte is deliberately kept: WqRec carries the GP0 op byte
// in wCol[0] >> 24, which is where billboardsRender's emitRecQuad reads the semi/raw bits from.
constexpr uint32_t COL_MASK = 0xFFF0F0F0u;

constexpr uint32_t SUB_TRANSFORM = 0x18u;   // 5 rotation words then 3 translation words
constexpr uint32_t SUB_GEOMBLK   = 0x40u;

// A geomblk with counts this large is not a geomblk — the same sanity bound mesh_draw.cpp uses.
constexpr int kMaxPrimsPerList = 4096;
// Backstop so a malformed node cannot flood the display-pass list. billboardsRender walks every
// record every frame, so an unbounded push here would be a frame-time cliff, not just wasted memory.
constexpr size_t kMaxWqRecs = 8192;

inline int16_t lo16(uint32_t v) { return (int16_t)(v & 0xFFFFu); }
inline int16_t hi16(uint32_t v) { return (int16_t)(v >> 16); }

}  // namespace

void Render::subPartCapture(Core* c, uint32_t node, uint32_t sub) {
  // The oracle leg runs the recompiled body and must produce no host-side picture state.
  if (c->game->oracle) return;

  const uint32_t geomblk = c->mem_r32(sub + SUB_GEOMBLK);
  if (!geomblk) return;
  const uint32_t counts = c->mem_r32(geomblk);
  const int n3 = (int)(counts & 0xFFFFu), n4 = (int)((counts >> 16) & 0xFFFFu);
  if (n3 > kMaxPrimsPerList || n4 > kMaxPrimsPerList) return;   // not a geomblk
  if (n3 == 0 && n4 == 0) return;

  Render* r = rend(c);
  if (r->mWqRecs.size() >= kMaxWqRecs) return;

  // This sub-part's transform, factored back to a WORLD transform so the display pass can re-compose
  // it with the (fps60-lerped) camera — identical to what the glyph pass does with cmd+0x18.
  float crF[3][3], tr[3], objR[3][3], objT[3];
  wq_read_matrix(c, sub + SUB_TRANSFORM, crF, tr);
  wq_factor_world(c, crF, tr, objR, objT);

  // Lerp identity key: (node, seq). Counting the node's existing records ONCE and then incrementing
  // keeps this O(n) per sub-part instead of O(n^2) per prim, and the walk order is deterministic
  // frame to frame, so the same prim gets the same seq in consecutive frames — which is what
  // billboardsRender matches on.
  uint32_t seq = 0;
  for (const Render::WqRec& p : r->mWqRecs) if (p.node == node) seq++;

  auto push = [&](const int16_t vx[4], const int16_t vy[4], const int16_t vz[4],
                  const uint32_t col[4], uint32_t uv0, uint32_t uv1, uint32_t uv2, uint32_t uv3) {
    if (r->mWqRecs.size() >= kMaxWqRecs) return;
    Render::WqRec w;
    w.node = node;
    w.seq  = seq++;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) w.objR[i][j] = objR[i][j];
      w.objT[i] = objT[i];
    }
    for (int i = 0; i < 4; i++) { w.vx[i] = vx[i]; w.vy[i] = vy[i]; w.vz[i] = vz[i]; w.wCol[i] = col[i]; }
    w.wUv0 = uv0; w.wUv1 = uv1; w.wUv2 = uv2; w.wUv3 = uv3;
    r->mWqRecs.push_back(w);
  };

  uint32_t rec = geomblk + 16;

  // ---- GT3 textured tris (36B) — 4th corner degenerates onto v2, the same split mesh_draw.cpp uses
  for (int t = 0; t < n3; t++, rec += 36) {
    const uint32_t xy0 = c->mem_r32(rec + 16), xy1 = c->mem_r32(rec + 24), xy2 = c->mem_r32(rec + 28);
    const uint32_t vz01 = c->mem_r32(rec + 20), z2w = c->mem_r32(rec + 32);
    const int16_t vx[4] = { lo16(xy0), lo16(xy1), lo16(xy2), lo16(xy2) };
    const int16_t vy[4] = { hi16(xy0), hi16(xy1), hi16(xy2), hi16(xy2) };
    const int16_t vz[4] = { lo16(vz01), hi16(vz01), lo16(z2w), lo16(z2w) };
    const uint32_t code0 = c->mem_r32(rec + 0), rgb1w = c->mem_r32(rec + 4);
    // rgb0 keeps the op byte (COL_MASK preserves it); rgb1 @+4; rgb2 = rgb1<<4; 4th mirrors the 3rd.
    const uint32_t rgb2 = (rgb1w << 4) & COL_MASK;
    const uint32_t col[4] = { code0 & COL_MASK, rgb1w & COL_MASK, rgb2, rgb2 };
    const uint32_t uv0 = c->mem_r32(rec + 8), uv1 = c->mem_r32(rec + 12);
    const uint32_t uv2 = c->mem_r16(rec + 34);      // uv2 lives in the HIGH half of the rec+32 word
    push(vx, vy, vz, col, uv0, uv1, uv2, uv2);
  }

  // ---- GT4 textured quads (44B) — the plank faces
  for (int q = 0; q < n4; q++, rec += 44) {
    const uint32_t xy0 = c->mem_r32(rec + 20), xy1 = c->mem_r32(rec + 28),
                   xy2 = c->mem_r32(rec + 32), xy3 = c->mem_r32(rec + 40);
    const uint32_t vz01 = c->mem_r32(rec + 24), vz23 = c->mem_r32(rec + 36);
    const int16_t vx[4] = { lo16(xy0), lo16(xy1), lo16(xy2), lo16(xy3) };
    const int16_t vy[4] = { hi16(xy0), hi16(xy1), hi16(xy2), hi16(xy3) };
    const int16_t vz[4] = { lo16(vz01), hi16(vz01), lo16(vz23), hi16(vz23) };
    const uint32_t code0 = c->mem_r32(rec + 0), code2 = c->mem_r32(rec + 4);
    const uint32_t col[4] = { code0 & COL_MASK, (code0 << 4) & COL_MASK,
                              code2 & COL_MASK, (code2 << 4) & COL_MASK };
    const uint32_t uv0 = c->mem_r32(rec + 8), uv1 = c->mem_r32(rec + 12), uv23 = c->mem_r32(rec + 16);
    push(vx, vy, vz, col, uv0, uv1, uv23 & 0xFFFFu, (uv23 >> 16) & 0xFFFFu);
  }

  // `PSXPORT_DEBUG=subpartcap` — does this fire at all, on which node, and how many prims did the
  // sub-part contribute? The first question any follow-up asks (a text-label node that never reaches
  // subPartWalk would make a silent no-op look like a working fix).
  if (cfg_dbg("subpartcap"))
    cfg_logf("subpartcap", "node=%08X sub=%08X geomblk=%08X gt3=%d gt4=%d recs=%zu",
             node, sub, geomblk, n3, n4, r->mWqRecs.size());
}
