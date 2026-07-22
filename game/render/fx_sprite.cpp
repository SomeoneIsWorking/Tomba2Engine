// game/render/fx_sprite.cpp — FxSprite: native display coverage for the FUN_80027A4C
// scaled-sprite packet writer (kanban #12 — torch flame missing under pc_render).
//
// RE (Ghidra scratch/decomp/fx_pkt_writer.c + fx_emit_leaves.c, ground truth generated/shard_5.c
// gen_func_80027A4C): 0x80027A4C is the ONE packet writer for the "world-anchored scaled sprite"
// effect family. Its three RE'd callers — FUN_80027CB4, FUN_80027E5C, FUN_800281EC (per-particle
// loop) — all follow the same contract:
//   1. load the pure camera CRs from scratchpad 0x1F8000F8, RTPS the node's world anchor
//      (node+0x2C/+0x30), range-check the OT depth,
//   2. publish the projection to scratchpad:
//        0x1F800080 (i32) OT bucket key   = quantize((SZ3>>2) + node+0x32 bias)
//        0x1F800084 (i32) X pixel scale   = IR0-derived (caller-specific modulation)
//        0x1F800088 (i32) Y pixel scale   (== X for these callers)
//        0x1F80008C/8E (s16,s16) screen anchor = SXY2
//        0x1F800090 (i32) depth-cue factor for DPCS (all three RE'd callers write 0)
//   3. call FUN_80027A4C(a0 = 8-byte record list, a1 = clut(lo16)|tpage(hi16)).
// 0x80027A4C then walks the records, each one 0x2C/0x2E textured quad:
//   word0 = 4 signed byte corner offsets {xL, xR, yT, yB}, scaled by the pixel scales >>16,
//           added to the screen anchor;
//   word1 = u(base|width) | v(base|height)<<8 | flags<<16 where flags:
//           bit0 semi (op 0x2C -> 0x2E AND the color low bit — mirrored as-is), bits1-2
//           tpage blend bits (<<4 into the tpage half), bits8-11 clut row offset (>>2 added
//           to the clut half), bit12 V flip, bit13 U flip, bits14-15 record-list terminator,
//           low byte = flat grey brightness;
//   color = DPCS(brightness, IR0=scratch 0x1F800090) — identity for IR0==0 (every RE'd caller).
//
// The torch flame at the seaside hut is a cluster of these records (packet nodes 0x800C9xxx,
// tp=(896,0) clut=(1008,192), attributed via `debug otattr` + REPL `otattr` to emitter leaf
// 0x800281EC under object node 0x800F06D8, beh FUN_8012D404 = beh_cull_tick_render). pc_render
// never walks the guest OT (break-first rebuild, 2026-07-15), so the family had NO native
// producer — the whole layer was absent.
//
// Fix shape = the sanctioned leaf tap (Panel::install / ScreenFade::installLeafTap precedent):
// run the ORIGINAL gen body (guest packet pool / OT / pool cursor stay byte-exact for SBS), then
// re-derive the same quads HOST-SIDE from the records + the scratchpad projection contract and
// hand them to the render queue as world prims with the object's flat view depth (the accepted
// #28/#39 billboard-class depth transcription: proj_pz_to_ord(SZ3), SZ3 = the caller's own RTPS
// result, still live in the GTE at call time). Read-only: no guest write outside gen.
#include "core.h"
#include "game.h"
#include "cfg.h"
#include "render_internal.h"   // ObjScope / cur_render_node / proj_pz_to_ord / render_queue.h

extern void gen_func_80027A4C(Core*);
#include "override_registry.h"

namespace {

// Scratchpad contract published by every FUN_80027A4C caller (RE'd above).
constexpr uint32_t kScrOtKey     = 0x1F800080u;
constexpr uint32_t kScrScaleX    = 0x1F800084u;
constexpr uint32_t kScrScaleY    = 0x1F800088u;
constexpr uint32_t kScrAnchorX   = 0x1F80008Cu;
constexpr uint32_t kScrAnchorY   = 0x1F80008Eu;
constexpr uint32_t kScrDepthCue  = 0x1F800090u;
constexpr uint32_t kGteSZ3       = 19u;          // GTE data reg 19 = SZ3 (the caller's RTPS depth)

void fxSpriteTap(Core* c) {
  const uint32_t rec0     = c->r[4];   // 8-byte record list
  const uint32_t clutPage = c->r[5];   // clut (lo16) | tpage (hi16)
  gen_func_80027A4C(c);                // byte-exact guest packet pool / OT / cursor
  if (c->game->oracle || c->rsub.mode.psxRender()) return;   // read-only overlay gate
  if (!rec0) return;

  const int anchorX = (int16_t)c->mem_r16(kScrAnchorX);
  const int anchorY = (int16_t)c->mem_r16(kScrAnchorY);
  const int32_t scaleX = (int32_t)c->mem_r32(kScrScaleX);
  const int32_t scaleY = (int32_t)c->mem_r32(kScrScaleY);
  const int32_t cue    = (int32_t)c->mem_r32(kScrDepthCue);
  const float od = proj_pz_to_ord((float)(int32_t)gte_read_data(kGteSZ3));
  const int ox = c->game->gpu.s_off_x, oy = c->game->gpu.s_off_y;

  // Whole-family flat object depth — same class as the obj_depth billboard transcription.
  float dep[4] = { od, od, od, od };
  ObjScope objScope(c, cur_render_node(c));   // prim identity (dbg_node) = the emitting object

  uint32_t rec = rec0;
  for (int guard = 0; guard < 256; guard++) {
    const uint32_t w0 = c->mem_r32(rec);
    const uint32_t w1 = c->mem_r32(rec + 4u);
    const uint32_t flags = w1 >> 16;

    // Corners: anchor + signed byte offset * scale >> 16 (gen truncates to s16 per store).
    const int xL = (int16_t)(anchorX + (int16_t)((uint32_t)((int32_t)(int8_t)(w0)       * scaleX) >> 16));
    const int xR = (int16_t)(anchorX + (int16_t)((uint32_t)((int32_t)(int8_t)(w0 >> 8)  * scaleX) >> 16));
    const int yT = (int16_t)(anchorY + (int16_t)((uint32_t)((int32_t)(int8_t)(w0 >> 16) * scaleY) >> 16));
    const int yB = (int16_t)(anchorY + (int16_t)((int32_t)((int32_t)w0 >> 24) * scaleY >> 16));

    // UVs with the guest's flip rules (note the asymmetric 7/6 insets — mirrored exactly).
    const uint32_t u = w1 & 0xFFu, v = (w1 >> 8) & 0xFFu;
    const int ub = (int)(u & 0xF8u), uw = (int)(u & 7u) * 8;
    const int vb = (int)(v & 0xF8u), vh = (int)(v & 7u) * 8;
    int u02, u13, v01, v23;
    if (!(flags & 0x2000u)) { u02 = ub + 1;      u13 = ub + uw + 7; }
    else                    { u02 = ub + uw + 6; u13 = ub + 1;      }
    if (!(flags & 0x1000u)) { v01 = vb + 1;      v23 = vb + vh + 7; }
    else                    { v01 = vb + vh + 7; v23 = vb + 1;      }

    // Flat grey brightness; DPCS depth-cue is identity for cue==0 (all RE'd callers). If a caller
    // ever publishes a non-zero cue, apply the DPCS lerp toward the GTE far color and say so once.
    int col = (int)(flags & 0xFFu);
    if (cue != 0) {
      static bool warned = false;
      if (!warned) { warned = true;
        cfg_logw("fxspr", "non-zero DPCS depth-cue factor %d — applying far-color lerp (unverified path)", cue); }
      const int fc = (int)((int32_t)gte_read_ctrl(21) >> 4);   // RFC (28.4) — grey packets use R for all
      col += (int)(((int64_t)cue * (fc - col)) >> 12);
      if (col < 0) col = 0; if (col > 255) col = 255;
    }

    const int semi = (int)(flags & 1u);
    const uint16_t clut  = (uint16_t)((clutPage & 0xFFFFu) + ((flags & 0xF00u) >> 2));
    const uint16_t tpage = (uint16_t)((clutPage >> 16) | ((flags & 6u) << 4));
    const int tp_x = (tpage & 0xF) * 64, tp_y = ((tpage >> 4) & 1) * 256;
    const int mode = (tpage >> 7) & 3, blend = (tpage >> 5) & 3;
    const int clut_x = (clut & 0x3F) * 16, clut_y = (clut >> 6) & 0x1FF;

    int xs[4] = { xL + ox, xR + ox, xL + ox, xR + ox };
    int ys[4] = { yT + oy, yT + oy, yB + oy, yB + oy };
    int us[4] = { u02, u13, u02, u13 };
    int vs[4] = { v01, v01, v23, v23 };
    unsigned char cc[4] = { (unsigned char)col, (unsigned char)col, (unsigned char)col, (unsigned char)col };
    c->game->activeRq().emitOrQueue(c, /*capture=*/1, RQ_WORLD, RQ_OM_DEPTH, 4, semi, /*raw=*/0,
                                    xs, ys, nullptr, nullptr, us, vs, cc, cc, cc, dep,
                                    mode, tp_x, tp_y, clut_x, clut_y,
                                    0, 0, 0, 0, 0, 0, 1023, 511, blend);

    rec += 8u;
    if (flags & 0xC000u) break;   // gen's post-condition: the terminal record IS emitted
  }
}

}  // namespace

void fx_sprite_install() {
  engine_set_override_main(0x80027A4Cu, fxSpriteTap, gen_func_80027A4C);
}
