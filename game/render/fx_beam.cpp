// game/render/fx_beam.cpp — NATIVE PRODUCER for the BEAM / SEE-SAW quad emitter (guest FUN_8003B704),
// one of the three `submitQuad` caller classes left picture-less by the 2026-08-04 tap retirement
// (docs/unported-render-inventory.md row R2, portmap step `render-producer-submitquad-classes`).
//
// WHAT IT DRAWS. A textured RIBBON stretched between the scene's tracked anchor object and the node's
// own world position — the plank of a see-saw, the shaft of a beam. The ribbon is one quad, or two
// quads meeting at the midpoint when the node asks to be split (kind == kKindSplit), which is what
// lets a long span bend correctly across the perspective divide.
//
// ────────────────────────────────────────────────────────────────────────────────────────────────
// RE — ground truth authenticated executable/overlay evidence `guest 0x8003B704`, read instruction by instruction.
// Ghidra headless was not needed here: the emitter has no COP2 ops of its own (it only *loads* the camera and hands the
// corners to FUN_8003B320), so the recorded guest instruction listing is already legible C and is the authority the
// project's own rules name for GTE-adjacent code.
//
//   1. HALF-EXTENT. Two node angles give a unit direction, scaled by kHalfWidth (0x14):
//        b = node.polar + (sceneMode < 4 ? kQuarterTurn : 0);  a = node.azimuth
//        r      = rcos(b) * kHalfWidth                       (Q12 * 20)
//        halfX  = (rcos(a) * r) >> 24        halfY = (rsin(b) * kHalfWidth) >> 12
//        halfZ  = (-rsin(a) * r) >> 24
//      i.e. H = 20 * (cos a cos b, sin b, -sin a cos b) — a world-space vector, computed from the
//      NODE's own angle fields. The integer shifts are reproduced exactly: they are the emitter's
//      own quantisation, not an approximation of it.
//
//   2. ENDPOINTS. A = the tracked anchor object *(0x800E7F5C), world position s32 at +0x2C/30/34.
//      N = this node's world position, the s16 integer halves of its 16.16 fields, at +0x2E/32/36.
//      kind == kKindSplit adds M = (A + N) / 2 (round toward zero) and draws A→M then M→N.
//
//   3. EACH SPAN P→Q becomes the quad  (P-H, Q-H, P+H, Q+H)  — so U runs along the span and V
//      across its width. Material: GP0 code 0x2D (textured, RAW: the colour word is never written,
//      the texture is not modulated), tpage half 5, clut 0x3E9F, U fixed to [kTexU0, kTexU1] and the
//      two V rows read from the 2-byte table entry at kBeamVTable[node.uvIndex].
//
//   4. The emitter loads the PURE CAMERA into the GTE control registers itself —
//      `SetRotMatrix(0x1F8000F8) / SetTransMatrix(0x1F8000F8)` (guest 0x80084660/80084690) — immediately
//      before building the corners. THAT SETTLES THE "CR CONTRACT" QUESTION docs/fps60-rework.md and
//      the inventory left open: whatever perObjRenderDispatch / billboardCompose1 left in the control
//      registers is overwritten here, so the corners this emitter builds ARE WORLD SPACE. The native
//      producer therefore projects them with the native (fps60-lerped) scene camera and nothing else.
//
// NOT A TAP. Every input above is the game's own state — two node angle fields, two world positions,
// a table index, a scene mode byte. No GTE register is read, no GP0 packet is inspected, no guest
// pre-composed matrix is factored, and no `original guest instructions` body runs to make the picture. (The guest's
// own packet emission still runs underneath and still feeds psx_render; this file only adds the
// native picture, and writes no guest memory.)
//
// DISPATCH. The guest reaches this emitter from FUN_8003EEC0's chain (render_walk.cpp's HEADS[2]),
// through its jump table at kW4Table: table[type] == kArmBeamMesh always calls it, table[type] ==
// kArmBeamBillboard calls it only when node+2 == 1. `beamNodeReached` below re-reads that same table
// and that same gate from guest memory rather than hardcoding a type list, so the producer follows
// the game's routing instead of a snapshot of it. (Dumped live 2026-08-06 for the record, NOT to be
// hardcoded: type 1 -> 0x8003EF30, type 16 -> 0x8003EF40, type 0 and 15 -> 0x8003EF20, type 32 ->
// 0x8003EF68, every other in-range type -> the 0x8003EF78 no-op. Type 32 mattering is why
// fieldObjectsRender's earlier `continue` on type-0x20 nodes hides no beam node.)
//
// DIAGNOSTIC. `PSXPORT_DEBUG=beamfx` prints one line per producer call — node, kind, uv index, both
// endpoints, the half-extent, and the screen bbox of what was actually emitted — plus a per-walk
// SUMMARY from render_walk.cpp carrying the denominator (how many objListWalk4 nodes were inspected
// and how many the guest's table routed here). A quiet channel with `inspected=0` means the chain was
// empty; `routed=0` means no node asked for a beam; `emitted=0` means the producer declined. Those
// are three different findings and the channel is built so they cannot be confused.
#include "core.h"
#include "game.h"
#include "game_ctx.h"    // trigOf(c)
#include "proj_params.h" // proj_pz_to_ord
#include "projection.h"
#include "render.h"
#include "render_internal.h" // ObjScope
#include "render_queue.h"
#include "trig.h" // Trig::rsin / rcos — the guest's own sine table, read natively
#include <cstdint>
#include <lucent/log.h>

namespace {

// ── the emitter's own constants ────────────────────────────────────────────────────────────────────
constexpr int32_t kHalfWidth = 0x14;          // the ribbon's half-width, in world units
constexpr int32_t kQuarterTurn = 1024;        // 4096 = a full turn, so this is 90 degrees
constexpr uint32_t kAnchorPtr = 0x800E7F5Cu;  // *this = the scene's tracked anchor object
constexpr uint32_t kSceneMode = 0x800E7FC6u;  // scene/view mode byte; < 4 turns the width axis 90 deg.
                                              // Its wider meaning is NOT RE'd — the emitter only ever
                                              // asks "< 4", and that is all this file claims about it.
constexpr uint32_t kBeamVTable = 0x800A3B04u; // 2 bytes per entry: the span's near/far V texel rows
constexpr int kTexU0 = 224;                   // the beam texture's column, [kTexU0, kTexU1]
constexpr int kTexU1 = 247;
constexpr uint16_t kTpageHalf = 5;  // GP0 tpage half-word written into the packet
constexpr uint16_t kClut = 0x3E9Fu; // CLUT half-word written into the packet
constexpr bool kRawTexture = true;  // GP0 code 0x2D: bit0 = raw, bit1 (semi-transparent) = 0
constexpr bool kSemi = false;

// The node's own fields (byte offsets), named rather than open-coded at the use site.
constexpr uint32_t kNodeKind = 0x60u;    // s16 — kKindSplit selects the two-span form
constexpr uint32_t kNodeUvIndex = 0x66u; // s16 — index into kBeamVTable
constexpr uint32_t kNodeAzimuth = 0x68u; // s16 — angle a
constexpr uint32_t kNodePolar = 0x6Au;   // s16 — angle b
constexpr uint32_t kNodePosX = 0x2Eu;    // s16 — integer half of the node's 16.16 world X (Y,Z at +4,+8)
constexpr uint32_t kNodeVisFlag = 0x02u; // u8  — the billboard arm's own "draw me" gate
constexpr int16_t kKindSplit = 3;

// The anchor object carries a PLAIN s32 world position, not the node's 16.16 pair — the emitter
// reads the full word for the midpoint and its low half for the corner, which agree exactly while
// the value fits in s16, and it always does for a world coordinate.
constexpr uint32_t kAnchorPosX = 0x2Cu; // s32 (Y,Z at +0x30, +0x34)

// FUN_8003EEC0's dispatch table and the two arms that reach this emitter.
constexpr uint32_t kW4Table = 0x80015000u;
constexpr uint32_t kW4TableEntries = 33u;
constexpr uint32_t kArmBeamMesh = 0x8003EF30u;      // perObjRenderDispatch, then the beam
constexpr uint32_t kArmBeamBillboard = 0x8003EF40u; // billboardCompose1, then the beam if node+2 == 1

// A world point in the guest's integer world units.
struct WorldPt {
  int32_t x, y, z;
};

// The emitter's midpoint: an integer halve that rounds toward zero, exactly as the guest writes it
// (`v += (unsigned)v >> 31; v >>= 1`).
inline int32_t halveTowardZero(int32_t v) {
  return (v + (int32_t)((uint32_t)v >> 31)) >> 1;
}

// Every corner the emitter builds lands in an s16 packet slot, so it wraps the same way.
inline int32_t s16of(int32_t v) {
  return (int16_t)(uint16_t)v;
}

} // namespace

// beamNodeReached — which of FUN_8003EEC0's arms this node takes, read from the GUEST's own jump
// table so the producer cannot drift from the routing. False when the node reaches no beam arm.
bool Render::beamNodeReached(uint32_t node) const {
  Core *c = mCore;
  const uint32_t type = c->mem_r8(node + 0x0Bu);
  if (type >= kW4TableEntries) {
    return false; // the walk's own range reject
  }
  const uint32_t arm = c->mem_r32(kW4Table + type * 4u);
  if (arm == kArmBeamMesh) {
    return true;
  }
  if (arm == kArmBeamBillboard) {
    return c->mem_r8(node + kNodeVisFlag) == 1u;
  }
  return false;
}

// beamQuadRender — FUN_8003B704's picture. Read-only; emits world quads with real per-vertex depth
// through the native camera, so the beam interpolates at fps60 like every other native producer.
void Render::beamQuadRender(uint32_t node) {
  Core *c = mCore;
  const uint32_t anchor = c->mem_r32(kAnchorPtr);
  if (!anchor) {
    return; // no tracked object: the emitter would read address 0x2C
  }

  // ── the half-extent vector, from the node's own two angles ──────────────────────────────────────
  const Trig &trig = trigOf(c);
  const int32_t azimuth = (int16_t)c->mem_r16(node + kNodeAzimuth);
  const int32_t polar = (int16_t)c->mem_r16(node + kNodePolar) + (c->mem_r8(kSceneMode) < 4u ? kQuarterTurn : 0);
  const int32_t spanR = trig.rcos(polar) * kHalfWidth; // the width vector's horizontal radius
  const WorldPt H = {
      (trig.rcos(azimuth) * spanR) >> 24, (trig.rsin(polar) * kHalfWidth) >> 12, (-trig.rsin(azimuth) * spanR) >> 24};

  // ── the two endpoints, and the midpoint the split form needs ────────────────────────────────────
  // The emitter reads the anchor TWICE and differently, and the port keeps that split rather than
  // tidying it: the MIDPOINT is built from the full 32-bit word, the CORNER from its low halfword.
  // The two agree exactly while the coordinate fits in s16 — which it does for every anchor observed
  // (A = (5917,-1050,3871) and the like) — but they are not the same read, so they are not merged.
  const WorldPt Amid = {(int32_t)c->mem_r32(anchor + kAnchorPosX),
                        (int32_t)c->mem_r32(anchor + kAnchorPosX + 4u),
                        (int32_t)c->mem_r32(anchor + kAnchorPosX + 8u)};
  const WorldPt A = {(int16_t)c->mem_r16(anchor + kAnchorPosX),
                     (int16_t)c->mem_r16(anchor + kAnchorPosX + 4u),
                     (int16_t)c->mem_r16(anchor + kAnchorPosX + 8u)};
  const WorldPt N = {(int16_t)c->mem_r16(node + kNodePosX),
                     (int16_t)c->mem_r16(node + kNodePosX + 4u),
                     (int16_t)c->mem_r16(node + kNodePosX + 8u)};
  const bool split = (int16_t)c->mem_r16(node + kNodeKind) == kKindSplit;
  const WorldPt M = {halveTowardZero(Amid.x + N.x), halveTowardZero(Amid.y + N.y), halveTowardZero(Amid.z + N.z)};

  // ── the material: fixed U column, the V row pair the node's table entry selects ─────────────────
  const int32_t uvIndex = (int16_t)c->mem_r16(node + kNodeUvIndex);
  const int vNear = c->mem_r8(kBeamVTable + (uint32_t)(uvIndex * 2));
  const int vFar = c->mem_r8(kBeamVTable + (uint32_t)(uvIndex * 2) + 1u);
  const int us[4] = {kTexU0, kTexU1, kTexU0, kTexU1};
  const int vs[4] = {vNear, vNear, vFar, vFar};

  EObjXform cam;
  projComposeCamera(&cam);
  ObjScope objScope(c, node);
  RenderQueue &rq = c->game->activeRq();

  // ── one span per quad: (P-H, Q-H, P+H, Q+H). Split form = A→M then M→N; plain form = A→N. ───────
  const int nSpans = split ? 2 : 1;
  const WorldPt spanFrom[2] = {A, M};
  const WorldPt spanTo[2] = {split ? M : N, N};
  int emitted = 0;
  float bx0 = 1e9f, by0 = 1e9f, bx1 = -1e9f, by1 = -1e9f; // screen bbox of what actually got emitted
  for (int s = 0; s < nSpans; s++) {
    const WorldPt &P = spanFrom[s];
    const WorldPt &Q = spanTo[s];
    const WorldPt corner[4] = {
        {s16of(P.x - H.x), s16of(P.y - H.y), s16of(P.z - H.z)},
        {s16of(Q.x - H.x), s16of(Q.y - H.y), s16of(Q.z - H.z)},
        {s16of(P.x + H.x), s16of(P.y + H.y), s16of(P.z + H.z)},
        {s16of(Q.x + H.x), s16of(Q.y + H.y), s16of(Q.z + H.z)},
    };
    ProjVtx p[4];
    bool behind = false;
    for (int i = 0; i < 4 && !behind; i++) {
      cam.project(corner[i].x, corner[i].y, corner[i].z, &p[i]);
      behind = p[i].sz <= 0; // the emitter's own GTE-flag reject
    }
    if (behind) {
      continue;
    }

    float xsf[4], ysf[4], depth[4];
    int xs[4], ys[4];
    for (int i = 0; i < 4; i++) {
      xsf[i] = p[i].px;
      ysf[i] = p[i].py;
      bx0 = bx0 < p[i].px ? bx0 : p[i].px;
      bx1 = bx1 > p[i].px ? bx1 : p[i].px;
      by0 = by0 < p[i].py ? by0 : p[i].py;
      by1 = by1 > p[i].py ? by1 : p[i].py;
      xs[i] = (int)(p[i].px < 0 ? p[i].px - 0.5f : p[i].px + 0.5f);
      ys[i] = (int)(p[i].py < 0 ? p[i].py - 0.5f : p[i].py + 0.5f);
      depth[i] = proj_pz_to_ord(p[i].pz);
    }
    // Raw texture (GP0 0x2D bit 0): the guest never writes the colour word, so the texel passes
    // through unmodulated — the neutral colour below is what "unmodulated" means to the rasterizer.
    const unsigned char neutral[4] = {0x80, 0x80, 0x80, 0x80};
    rq.emitOrQueue(c,
                   /*capture=*/1,
                   RQ_WORLD,
                   RQ_OM_DEPTH,
                   /*nv=*/4,
                   kSemi ? 1 : 0,
                   kRawTexture ? 1 : 0,
                   xs,
                   ys,
                   xsf,
                   ysf,
                   us,
                   vs,
                   neutral,
                   neutral,
                   neutral,
                   depth,
                   /*mode=*/(int)((kTpageHalf >> 7) & 3u),
                   /*tp_x=*/(int)(kTpageHalf & 0xFu) * 64,
                   /*tp_y=*/(int)((kTpageHalf >> 4) & 1u) * 256,
                   /*clut_x=*/(int)(kClut & 0x3Fu) * 16,
                   /*clut_y=*/(int)((kClut >> 6) & 0x1FFu),
                   0,
                   0,
                   0,
                   0,
                   0,
                   0,
                   1023,
                   511,
                   /*tp_blend=*/(int)((kTpageHalf >> 5) & 3u));
    emitted++;
  }
  lucent::debug("beamfx",
                "node={:08X} kind={} split={} uvIdx={} v=({},{}) A=({},{},{}) N=({},{},{}) "
                "H=({},{},{}) spans={} emitted={} screen=[{:.1f},{:.1f}]..[{:.1f},{:.1f}]",
                node,
                (int16_t)c->mem_r16(node + kNodeKind),
                split ? 1 : 0,
                uvIndex,
                vNear,
                vFar,
                A.x,
                A.y,
                A.z,
                N.x,
                N.y,
                N.z,
                H.x,
                H.y,
                H.z,
                nSpans,
                emitted,
                emitted ? bx0 : 0.0f,
                emitted ? by0 : 0.0f,
                emitted ? bx1 : 0.0f,
                emitted ? by1 : 0.0f);
}
