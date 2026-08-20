// game/render/fx_rope_strip.cpp — NATIVE PRODUCER for the TILED VERTICAL QUAD STRIP emitter
// (guest FUN_801365C4), the rope/cable/chain class.
//
// WHY THIS FILE EXISTS. kanban #103: the USER reported a cutscene whose MACHINERY is invisible and
// whose BRIDGE ROPES are missing under pc_render. The machine's MESH turned out to be drawn fine —
// measured, not assumed: the `redirdiag` census (perobj_dispatch.cpp) accounts for 100.00% of
// 2,615,850 per-object cmds across a 31,227-frame replay, with zero unrecognised emitters and zero
// draws that emitted nothing. What is missing is a SECOND draw the same object makes.
//
// docs/unported-render-inventory.md ranks FUN_801365C4 fifth on the global work list: 87,224 guest
// prims across 29,085 frames of one session, no native producer. It reaches the picture only through
// FUN_8003B320 (game/render/quad_rtpt_submit.cpp), whose own banner already names this debt —
// "NO pc_render PICTURE FROM THIS LEAF" — because that leaf is a faithful substrate mirror that fills
// the guest packet and OT-links it and pushes not one native primitive. So every object whose only
// draw goes through it is structurally absent from pc_render.
//
// ────────────────────────────────────────────────────────────────────────────────────────────────
// RE — ground truth generated/ov_a00_shard_1.c `ov_a00_gen_801365C4`, read instruction by
// instruction, plus its ONE caller `ov_a00_gen_80136748` (generated/ov_a00_shard_0.c:20083) for the
// transform. There are no COP2 ops in either body — both only *set up* the GTE and hand corners to
// FUN_8003B320 — so the recompiler's translation is legible C and is the authority the project's own
// rules name for GTE-adjacent code.
//
//   1. THE STRIP. a0 = node, a1 = LENGTH. The emitter divides the length by kSegment (120):
//      quotient = whole segments, remainder = the leftover tail. It then emits `quotient` quads,
//      stepping Y down by kSegment each time, and one final quad covering the remainder — so the
//      strip runs from y = length at the top down to y = 0, in fixed 120-unit tiles. Every tile
//      carries the SAME four UVs, which is what makes it a repeating rope/chain texture rather than
//      one stretched sheet.
//
//   2. EACH TILE, in model space, is the flat quad
//         v0 = (0, yTop,            -halfWidth)     v1 = (0, yTop,            +halfWidth)
//         v2 = (0, yTop - kSegment, -halfWidth)     v3 = (0, yTop - kSegment, +halfWidth)
//      X is written as literal zero for all four corners; the width is node+0x6C halved toward zero,
//      applied on Z. Standard PSX quad order (tris 0-1-2 and 1-2-3), which is the order the render
//      queue takes as well.
//
//   3. MATERIAL. GP0 code 0x2D — textured quad, RAW (bit 0 set: the texel is not modulated), NOT
//      semi-transparent (bit 1 clear). The colour word is written (128,128,128), which is exactly
//      what "unmodulated" means to the rasterizer. tpage = node+0x60, clut = node+0x62, and the four
//      UVs are node+0x64/0x66/0x68/0x6A, one packed (u,v) byte pair each.
//
//   4. THE TRANSFORM, and this is the part that settles whether a producer is even possible. The
//      caller FUN_80136748 composes the GTE itself, immediately before the call:
//         rec = mem32(node+200);  WORLD_POS = (mem16(rec+44), mem16(rec+48), mem16(rec+52) + 46)
//         FUN_80084470(CAM_ROT, WORLD_POS, mat)     -> mat.t = CamRot * WORLD_POS
//         mat.t += CAM_TRANS
//         FUN_80084660(CAM_ROT)  -> CR0-4 = the PURE CAMERA rotation (no object rotation at all)
//         FUN_80084690(mat)      -> CR5-7 = that composed translation
//      So a model corner M projects as  CamRot*M + (CamRot*P + CamTrans)  =  CamRot*(M + P) + CamTrans,
//      i.e. THE CORNERS ARE WORLD SPACE, offset from the object's own world position P. Same
//      situation FUN_8003B704's beams were in, and this producer answers it the same way
//      (game/render/fx_beam.cpp is the precedent, deliberately followed line for line).
//
// NOT A TAP. Every input is the game's own state read from guest RAM — a record pointer at node+200,
// three world coordinates, a length, a width, a tpage/clut pair and four UVs. No GTE register is
// read back, no GP0 packet is inspected, no pre-composed guest matrix is factored. Reading the
// transform out of the GTE control registers is what the USER banned outright on 2026-08-04
// ("never do this please NEVER"), and it is precisely what this file avoids by re-deriving P from
// the node instead.
//
// THE GUEST'S OWN EMISSION IS UNTOUCHED. ropeStrip() runs the real ov_a00_gen_801365C4 body first
// and only then adds the native picture, so psx_render, the oracle and SBS all see exactly what they
// saw before; nothing here writes guest memory.
//
// SCOPE, stated honestly: FUN_801365C4 has exactly ONE caller in the whole recompiled image
// (FUN_80136748), so this producer covers that caller class and no other. The transform contract is
// the CALLER's, not the emitter's, which is why it cannot be generalised to the other ten callers of
// FUN_8003B320 without RE'ing each of them — see kanban #103 for the shape that would.
//
// DIAGNOSTIC. `PSXPORT_DEBUG=ropefx` prints one line per producer call carrying the DENOMINATOR:
// node, length, the segment split, the world anchor, and how many of the tiles actually reached the
// queue. `tiles=N emitted=0` is a real, distinguishable finding (every tile rejected behind the
// camera) and reads differently from the producer never being called at all, which prints nothing.
#include "core.h"
#include "game.h"
#include "game_ctx.h"
#include "render.h"
#include "render_queue.h"
#include "render_internal.h"   // ObjScope, render_field_native_active
#include "gpu_native_internal.h"   // gpu_frame_no — declared THERE, never re-declared here
#include "projection.h"
#include "proj_params.h"       // proj_pz_to_ord
#include <lucent/log.h>
#include <cstdint>

extern void ov_a00_gen_801365C4(Core*);

namespace {

// ── the emitter's own constants, named rather than open-coded at the use site ──────────────────────
constexpr int32_t  kSegment   = 120;    // one texture tile's height in world units; also the divisor
constexpr int32_t  kOtzBias   = 32;     // a2 to FUN_8003B320 — the substrate's OT bias, mirrored in
                                        // spirit by the native depth sort rather than reproduced
constexpr bool     kRawTexture = true;  // GP0 code 0x2D bit 0
constexpr bool     kSemi       = false; // GP0 code 0x2D bit 1 is clear

// The node's own fields (byte offsets).
constexpr uint32_t kNodeTpage  = 0x60u;   // u16
constexpr uint32_t kNodeClut   = 0x62u;   // u16
constexpr uint32_t kNodeUv0    = 0x64u;   // u16 each, packed (u | v<<8), one per corner
constexpr uint32_t kNodeWidth  = 0x6Cu;   // s16 — FULL width; the emitter halves it toward zero
constexpr uint32_t kNodeRecPtr = 200u;    // *this = the record carrying the world anchor

// The anchor record's world position, and the emitter's own +46 Z bias (a literal in FUN_80136748).
constexpr uint32_t kRecPosX    = 44u;     // s16 (Y at +48, Z at +52)
constexpr int32_t  kRecZBias   = 46;

// Halve toward zero, exactly as the guest writes it (`v += (unsigned)v >> 31; v >>= 1`).
inline int32_t halveTowardZero(int32_t v) { return (v + (int32_t)((uint32_t)v >> 31)) >> 1; }

// Every corner the emitter builds lands in an s16 packet slot, so it wraps the same way.
inline int32_t s16of(int32_t v) { return (int16_t)(uint16_t)v; }

}  // namespace

// ropeStripRender — FUN_801365C4's picture. Read-only; emits world quads with real per-vertex depth
// through the native camera, so the strip interpolates at fps60 like every other native producer.
void Render::ropeStripRender(uint32_t node, int32_t length) {
  Core* c = mCore;

  const uint32_t rec = c->mem_r32(node + kNodeRecPtr);
  if (!rec) {
    // The emitter's caller would have read through address 0x2C. Say so — a producer that returns
    // silently here is indistinguishable from one that was never reached.
    lucent::debug("ropefx", "f{} node={:08X} len={} DECLINED: node+200 record pointer is null",
                  gpu_frame_no(c), node, length);
    return;
  }

  // ── the world anchor, from the node's own record ────────────────────────────────────────────────
  const int32_t px = (int16_t)c->mem_r16(rec + kRecPosX);
  const int32_t py = (int16_t)c->mem_r16(rec + kRecPosX + 4u);
  const int32_t pz = (int16_t)c->mem_r16(rec + kRecPosX + 8u) + kRecZBias;

  // ── the tiling: whole segments, then the remainder ──────────────────────────────────────────────
  // MIRRORS THE GUEST'S DIVISION, including its sign behaviour: the emitter sign-extends the length
  // to 32 bits and divides by 120, so a negative length yields a non-positive quotient and the whole
  // loop is skipped, leaving only the remainder tile. Reproduced rather than guarded away.
  const int32_t len  = (int16_t)(uint16_t)length;
  const int32_t whole = len / kSegment;
  const int32_t rem   = len % kSegment;

  const int32_t half = halveTowardZero((int16_t)c->mem_r16(node + kNodeWidth));

  // ── the material ────────────────────────────────────────────────────────────────────────────────
  const uint16_t tpage = c->mem_r16(node + kNodeTpage);
  const uint16_t clut  = c->mem_r16(node + kNodeClut);
  int us[4], vs[4];
  for (int i = 0; i < 4; i++) {
    const uint16_t uv = c->mem_r16(node + kNodeUv0 + (uint32_t)i * 2u);
    us[i] = uv & 0xFFu;          // PSX packs a UV slot as u in the low byte, v in the high byte
    vs[i] = (uv >> 8) & 0xFFu;
  }

  EObjXform cam;
  projComposeCamera(&cam);
  ObjScope objScope(c, node);
  RenderQueue& rq = c->game->activeRq();

  // ── one quad per tile, top-down, exactly as the emitter walks it ────────────────────────────────
  // The guest emits `whole` full tiles stepping Y down by kSegment, then one tile spanning the
  // remainder. Tile i therefore covers [yTop - height, yTop] with yTop = len - i*kSegment.
  const int32_t tiles = (whole > 0 ? whole : 0) + 1;
  int emitted = 0, behindCount = 0;
  float bx0 = 1e9f, by0 = 1e9f, bx1 = -1e9f, by1 = -1e9f;   // screen bbox of what actually got emitted
  for (int32_t i = 0; i < tiles; i++) {
    const int32_t yTop    = len - i * kSegment;
    const bool    lastOne = (i == tiles - 1);
    const int32_t height  = lastOne ? rem : kSegment;
    const int32_t yBot    = yTop - height;

    // World-space corners: the model quad (0, y, +/-half) offset by the object's world position.
    const int32_t cx[4] = { px, px, px, px };
    const int32_t cy[4] = { s16of(yTop) + py, s16of(yTop) + py, s16of(yBot) + py, s16of(yBot) + py };
    const int32_t cz[4] = { s16of(-half) + pz, s16of(half) + pz, s16of(-half) + pz, s16of(half) + pz };

    ProjVtx p[4];
    bool behind = false;
    for (int k = 0; k < 4 && !behind; k++) {
      cam.project(cx[k], cy[k], cz[k], &p[k]);
      behind = p[k].sz <= 0;                 // the emitter's own GTE-flag reject
    }
    if (behind) { behindCount++; continue; }

    float xsf[4], ysf[4], depth[4];
    int xs[4], ys[4];
    for (int k = 0; k < 4; k++) {
      xsf[k] = p[k].px; ysf[k] = p[k].py;
      bx0 = bx0 < p[k].px ? bx0 : p[k].px;  bx1 = bx1 > p[k].px ? bx1 : p[k].px;
      by0 = by0 < p[k].py ? by0 : p[k].py;  by1 = by1 > p[k].py ? by1 : p[k].py;
      xs[k] = (int)(p[k].px < 0 ? p[k].px - 0.5f : p[k].px + 0.5f);
      ys[k] = (int)(p[k].py < 0 ? p[k].py - 0.5f : p[k].py + 0.5f);
      depth[k] = proj_pz_to_ord(p[k].pz);
    }
    // Raw texture (GP0 0x2D bit 0): the texel passes through unmodulated, and the neutral colour
    // below is what "unmodulated" means to the rasterizer — the same (128,128,128) the guest writes.
    const unsigned char neutral[4] = { 0x80, 0x80, 0x80, 0x80 };
    rq.emitOrQueue(c, /*capture=*/1, RQ_WORLD, RQ_OM_DEPTH, /*nv=*/4, kSemi ? 1 : 0, kRawTexture ? 1 : 0,
                   xs, ys, xsf, ysf, us, vs, neutral, neutral, neutral, depth,
                   /*mode=*/(int)((tpage >> 7) & 3u),
                   /*tp_x=*/(int)(tpage & 0xFu) * 64, /*tp_y=*/(int)((tpage >> 4) & 1u) * 256,
                   /*clut_x=*/(int)(clut & 0x3Fu) * 16, /*clut_y=*/(int)((clut >> 6) & 0x1FFu),
                   0, 0, 0, 0, 0, 0, 1023, 511, /*tp_blend=*/(int)((tpage >> 5) & 3u));
    emitted++;
  }

  // DENOMINATOR ON EVERY RATIO. tiles is what the emitter asked for, emitted is what reached the
  // queue, behind is why the difference exists. "tiles=9 emitted=0 behind=9" and the producer never
  // running at all are different findings and this line keeps them different.
  lucent::debug("ropefx", "f{} node={:08X} len={} -> {} whole + rem {} = {} tiles | half={} P=({},{},{}) "
                          "tpage={:04X} clut={:04X} | emitted={} behind={} screen=[{:.1f},{:.1f}]..[{:.1f},{:.1f}]",
                gpu_frame_no(c), node, len, whole, rem, tiles, half, px, py, pz, tpage, clut,
                emitted, behindCount,
                emitted ? bx0 : 0.0f, emitted ? by0 : 0.0f, emitted ? bx1 : 0.0f, emitted ? by1 : 0.0f);
}

namespace {
// The override body: the guest's own emission first (untouched — psx_render, the oracle and SBS must
// see exactly what they saw before), then the native picture, and only inside pc_render's native
// field-pass window. Outside that window the full guest-OT walk IS the picture, so adding a native
// draw there would double-draw — the same gate, for the same reason, as the per-object redirect.
void ov_ropeStrip(Core* c) {
  const uint32_t node   = c->r[4];
  const int32_t  length = (int32_t)c->r[5];
  ov_a00_gen_801365C4(c);                       // unchanged substrate emission
  if (!render_field_native_active(c)) return;
  DisplayPassGuard displayPass(c->rsub.mode);   // this addition is display-pass only: no guest writes
  rend(c)->ropeStripRender(node, length);
}
}  // namespace

void fx_rope_strip_install() {
  extern void engine_set_override_a00(uint32_t, OverrideFn, OverrideFn);
  engine_set_override_a00(0x801365C4u, ov_ropeStrip, ov_a00_gen_801365C4);
}
