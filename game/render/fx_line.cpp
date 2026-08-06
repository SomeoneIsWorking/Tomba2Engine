// game/render/fx_line.cpp — NATIVE PRODUCER for the engine's WORLD-LINE family: ropes, chains and
// tethers (kanban #56 systemic, #54 the bucket's rope).
//
// THE GAP (measured, 2026-07-23): the GP0 interpreter handles line primitives (gpu_native.cpp, op
// 0x40..0x5F) so every rope shows up under psx_render — but the native render queue deals in
// triangles/quads only, and since pc_render stopped walking the guest OT there was no producer for a
// line at all. Every rope, chain and cable in the game was therefore invisible under pc_render: the
// bucket's suspension rope (#54), the fisherman's line, the hanging vines.
//
// RE (Ghidra on a live seaside RAM dump, scratch/decomp/lines*.c; census from `debug otattr,lineprim`
// on replays/bugs/bucket-softlock.pad). Every 0x5E line packet in the scene comes from ONE shared leaf:
//
//   FUN_8013DD34(A, B) — "draw a rope between two WORLD points". It builds the world MIDPOINT
//   M = (A+B)/2, RTPTs the triple (A, M, B) in one go, and emits a FOUR-point gouraud poly-line
//   P0, (P0+P1)/2, (P1+P2)/2, P2 — i.e. the projected chain with its middle corner cut, so a rope that
//   bends across the perspective divide reads as a smooth curve instead of a kink. The colour is a
//   16-entry grey ramp at 0x8014BD04 indexed by (SX0+SY0)&0xF and stepped by 4 per vertex, which is
//   what gives a rope its woven shimmer. It emits the poly-line TWICE, the second copy one screen pixel
//   lower — that is the guest's way of making a 1px line into a 2px stroke.
//
// Three callers, all ported here, each reading its own object's state:
//   * FUN_8013E9D8  — one rope from the object at node+0x14 down to the node's own world position.
//                     THE BUCKET's rope (#54): the bucket node hangs off the pole node it points at.
//   * FUN_8013EA64  — a 7-segment CHAIN through the node's own point array at node+0x60 (stride 4),
//                     with the fixed third axis at node+0x36 or node+0x2E chosen by node+3.
//   * FUN_80122974  — a TETHER with four anchor modes selected by node+0x47: straight down 400 units,
//                     a fixed world anchor, an offset carried in node+0x80/0x84, or an EIGHT-segment
//                     chain drawn toward the tracked object at 0x800BF868 (the fishing line).
//
// PORTED, NOT TAPPED: every point is read from the emitting object's own state and projected through
// projComposeCamera — the fps60-lerped scene camera — so the ropes interpolate like every other native
// producer. No gen body runs for the picture, no gte_op, no guest write. The screen-space expansion of
// a segment into a quad lives HERE, in the producer, so the render queue stays quads-only.
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include "render_internal.h"   // ObjScope
#include "proj_params.h"       // proj_pz_to_ord
#include <lucent/log.h>        // `ropeline` diagnostic channel
#include <math.h>

int gpu_frame_no(Core*);       // present-frame counter (gpu_native.cpp)

namespace {

// --- the shared rope leaf's own data (A00-family overlay, live while its callers are) --------------
constexpr uint32_t kGreyRamp   = 0x8014BD04u;   // 16-entry grey ramp: the rope's woven shimmer
constexpr int      kRampStep   = 4;             // ramp index advance per chain vertex
constexpr int      kRampMask   = 0xF;

// The guest draws the poly-line twice, the second copy one screen pixel below the first, so a rope's
// footprint is two pixels across. The native producer draws ONE stroke of that width instead, expanded
// perpendicular to each segment (which also keeps it even on the diagonals, where stacking two
// axis-aligned 1px lines leaves a stepped edge).
constexpr float kStrokePx = 2.0f;

// The rope packets are semi-transparent and the emitter's own DR_MODE selects blend mode 3 (B + F/4) —
// confirmed live by the `lineprim` census, which reports the GPU blend in force when each line draws.
constexpr int kRopeBlend = 3;

// --- FUN_80122974's node fields -------------------------------------------------------------------
constexpr uint32_t kTetherMode = 0x47u;   // anchor-mode selector
// The ROPE/TETHER node family's own world position, three s16 at 0x4E/0x50/0x52. NOT a universal node
// layout — do not reach for it from another producer without checking what that emitter reads. The
// shockwave ring below took its translation from here and drew nothing for nine days: on a ring node
// 0x50 is the scale animator, and that emitter reads a packed SVECTOR at 0x2C instead (claim C036/C038).
constexpr uint32_t kOwnPosX    = 0x4Eu;
constexpr uint32_t kTetherOffX = 0x80u;   // mode 2: X offset from the node
constexpr uint32_t kTetherOffY = 0x84u;   // mode 2: absolute Y (the +100 lands below it)
constexpr int      kTetherDrop = 400;     // mode 0: how far the tether hangs
constexpr int      kTetherRise = 100;     // mode 2: the bias added to the Y it carries
constexpr int      kTetherSegs = 8;       // mode 3: the chain's segment count

// Mode 1's fixed world anchor — three s16s, each in the HIGH half of its word.
constexpr uint32_t kFixedAnchorX = 0x800E7EAEu;
constexpr uint32_t kFixedAnchorY = 0x800E7EB2u;
constexpr uint32_t kFixedAnchorZ = 0x800E7EB6u;

// Mode 3's tracked object (the pointer the field keeps to whatever the tether follows) and the world
// position inside it.
constexpr uint32_t kTrackedPtr = 0x800BF868u;
constexpr uint32_t kTrackedX = 0x2Eu, kTrackedY = 0x32u, kTrackedZ = 0x36u;

// --- FUN_8013EA64's node fields -------------------------------------------------------------------
constexpr uint32_t kChainAxisFlag = 0x03u;   // 0 = the chain runs in X, 1..2 = it runs in Z
constexpr uint32_t kChainPts      = 0x60u;   // 8 (horizontal, Y) s16 pairs, stride 4
constexpr uint32_t kChainFixedZ   = 0x36u;   // the fixed axis when the chain runs in X
constexpr uint32_t kChainFixedX   = 0x2Eu;   // ... and when it runs in Z
constexpr int      kChainPts_N    = 8;

// --- FUN_8013E9D8's node fields -------------------------------------------------------------------
constexpr uint32_t kAnchorObjPtr = 0x14u;    // pointer to the object this one hangs from (its s16 XYZ)

// A world point in the guest's s16 coordinates.
struct WorldPt { int x, y, z; };

// The guest's divide-by-8 with its round-toward-zero fixup (`if (n < 0) n += 7; n >>= 3`).
inline int div8(int n) { return (n < 0 ? n + 7 : n) >> 3; }

// Every derived coordinate lands back in an s16 slot in the guest, so wrap the same way it does.
inline int s16(int n) { return (int16_t)(uint16_t)n; }

// --- the shockwave ring (FUN_8013E08C) ------------------------------------------------------------
constexpr uint32_t kRingTable = 0x8014C780u;  // 15 points, 2 words each (packed VXY, then VZ)
constexpr int      kRingSpans = 7;            // 7 overlapping 3-point spans cover all 15 points

// The ring node's own fields, at the offsets the emitter itself reads (see the RE banner on
// Render::shockwaveRingRender for the instructions these come from).
constexpr uint32_t kRingPos   = 0x2Cu;   // SVECTOR world position: X@0x2C, Y@0x2E, Z@0x30
constexpr uint32_t kRingScale = 0x50u;   // s16: both the ring's radius scale AND its fade input

// The fade the guest derives from the SAME field as the radius: as the ring grows from 0x14 to 0xDC
// its grey falls from 0x80 to 0. One animator drives both, which is why a shockwave dims as it spreads.
constexpr int kRingFadeBase = 0x80, kRingFadeStart = 0x14, kRingFadeSpan = 200;

// The doubled stroke. The guest emits each span TWICE, each with its own DR_MODE tpage: an unshifted
// copy in PSX blend mode 1 (B+F, an additive HIGHLIGHT) and a copy displaced (+2,+1) in blend mode 2
// (B-F, a subtractive SHADOW), the shadow drawn underneath. That pairing is the ring's embossed edge —
// it is not a 2px stroke, and drawing both copies in one blend mode (as this producer first did) loses
// the effect entirely.
constexpr int   kRingHighlightBlend = 1, kRingShadowBlend = 2;
constexpr float kRingShadowDx = 2.0f, kRingShadowDy = 1.0f;

// A shockwave-ring render node, read the way its emitter reads it.
struct ShockwaveRingNode {
  Core* c;
  uint32_t addr;
  // The animator: radius scale in 1.3.12 once shifted left 4, and the fade input.
  int scale() const { return (int16_t)c->mem_r16(addr + kRingScale); }
  // The node's own world position. The emitter hands these two words to the GTE as VXY0/VZ0, so this
  // is a packed SVECTOR (X and Y sharing one word), NOT the stride-4 s16 triple a render COMMAND uses.
  WorldPt worldPos() const {
    const uint32_t xy = c->mem_r32(addr + kRingPos), z = c->mem_r32(addr + kRingPos + 4);
    return WorldPt{ (int16_t)xy, (int16_t)(xy >> 16), (int16_t)z };
  }
};

// grey = 0x80 - ((scale - 0x14) * 0x80) / 200, truncating toward zero exactly as the guest's divide
// does. The guest does NOT clamp: it ORs the result straight into its packet header, so a value
// outside 0..0xFF would corrupt the primitive code — i.e. staying in range is a guest invariant, and
// the measured range on bucket-softlock is 13..122. The clamp here keeps a native colour byte
// well-formed instead of wrapping if that invariant is ever broken.
inline unsigned char ringFadeGrey(int scale) {
  int v = kRingFadeBase - ((scale - kRingFadeStart) * kRingFadeBase) / kRingFadeSpan;
  if (v < 0) v = 0; else if (v > 0xFF) v = 0xFF;
  return (unsigned char)v;
}

// One screen-space segment of a monochrome GPU line, drawn as the thin quad the native queue takes.
// A GP0 line is 1px wide, so this uses a 1px stroke rather than the rope's kStrokePx.
inline void strokeSegment(Core* c, RenderQueue& rq, const ProjVtx& a, const ProjVtx& b,
                          float ox, float oy, unsigned char grey, int blend) {
  const float ax = a.px + ox, ay = a.py + oy, bx = b.px + ox, by = b.py + oy;
  const float dx = bx - ax, dy = by - ay;
  const float len = sqrtf(dx * dx + dy * dy);
  if (len < 1e-4f) return;                                  // degenerate: the GPU would draw nothing
  const float nx = -dy / len * 0.5f, ny = dx / len * 0.5f;  // half of a 1px stroke
  const float xsf[4] = { ax + nx, bx + nx, ax - nx, bx - nx };
  const float ysf[4] = { ay + ny, by + ny, ay - ny, by - ny };
  const float da = proj_pz_to_ord(a.pz), db = proj_pz_to_ord(b.pz);
  const float depth[4] = { da, db, da, db };
  int xs[4], ys[4];
  for (int i = 0; i < 4; i++) {
    xs[i] = (int)(xsf[i] < 0 ? xsf[i] - 0.5f : xsf[i] + 0.5f);
    ys[i] = (int)(ysf[i] < 0 ? ysf[i] - 0.5f : ysf[i] + 0.5f);
  }
  const unsigned char col[4] = { grey, grey, grey, grey };
  const int uv[4] = { 0, 0, 0, 0 };
  rq.emitOrQueue(c, /*capture=*/1, RQ_WORLD, RQ_OM_DEPTH, /*nv=*/4, /*semi=*/1, /*raw=*/0,
                 xs, ys, xsf, ysf, uv, uv, col, col, col, depth,
                 /*mode=*/3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1023, 511, blend);
}

}  // namespace

// FUN_8013DD34 — THE rope leaf: a stroke between two world points, drawn as the projected 4-point
// chain the guest builds. Read-only; emits screen-space quads with real per-vertex depth.
void Render::worldLineDraw(int ax, int ay, int az, int bx, int by, int bz) {
  Core* c = mCore;
  EObjXform cam; projComposeCamera(&cam);

  // The guest RTPTs (A, midpoint, B) together — the midpoint is what lets the rope curve.
  const int mx = (ax + bx) >> 1, my = (ay + by) >> 1, mz = (az + bz) >> 1;
  ProjVtx pa, pm, pb;
  cam.project(ax, ay, az, &pa);
  cam.project(mx, my, mz, &pm);
  cam.project(bx, by, bz, &pb);
  if (pa.sz <= 0 || pm.sz <= 0 || pb.sz <= 0) return;   // behind the camera: the emitter's own reject

  // The corner-cut chain: the ends stay put, the two inner points sit halfway to the projected midpoint.
  const float cx[4] = { pa.px, (pa.px + pm.px) * 0.5f, (pb.px + pm.px) * 0.5f, pb.px };
  const float cy[4] = { pa.py, (pa.py + pm.py) * 0.5f, (pb.py + pm.py) * 0.5f, pb.py };
  const float cd[4] = { proj_pz_to_ord(pa.pz), proj_pz_to_ord((pa.pz + pm.pz) * 0.5f),
                        proj_pz_to_ord((pb.pz + pm.pz) * 0.5f), proj_pz_to_ord(pb.pz) };

  // The grey ramp, walked from the first vertex's screen position — the rope's shimmer.
  int rampIdx = (pa.sx + pa.sy) & kRampMask;
  unsigned char col[4];
  for (int i = 0; i < 4; i++) { col[i] = c->mem_r8(kGreyRamp + (uint32_t)rampIdx); rampIdx = (rampIdx + kRampStep) & kRampMask; }

  RenderQueue& rq = c->game->activeRq();
  const float half = kStrokePx * 0.5f;
  for (int s = 0; s + 1 < 4; s++) {
    const float dx = cx[s + 1] - cx[s], dy = cy[s + 1] - cy[s];
    const float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-4f) continue;                         // a degenerate segment draws nothing
    const float nx = -dy / len * half, ny = dx / len * half;   // the stroke's screen-space normal
    // Quad vertex order is the queue's own Z layout: tris (0,1,2) and (1,2,3).
    const float xsf[4] = { cx[s] + nx, cx[s + 1] + nx, cx[s] - nx, cx[s + 1] - nx };
    const float ysf[4] = { cy[s] + ny, cy[s + 1] + ny, cy[s] - ny, cy[s + 1] - ny };
    const float depth[4] = { cd[s], cd[s + 1], cd[s], cd[s + 1] };
    int xs[4], ys[4];
    for (int i = 0; i < 4; i++) {
      xs[i] = (int)(xsf[i] < 0 ? xsf[i] - 0.5f : xsf[i] + 0.5f);
      ys[i] = (int)(ysf[i] < 0 ? ysf[i] - 0.5f : ysf[i] + 0.5f);
    }
    const unsigned char rr[4] = { col[s], col[s + 1], col[s], col[s + 1] };
    const int uv[4] = { 0, 0, 0, 0 };
    rq.emitOrQueue(c, /*capture=*/1, RQ_WORLD, RQ_OM_DEPTH, /*nv=*/4, /*semi=*/1, /*raw=*/0,
                   xs, ys, xsf, ysf, uv, uv, rr, rr, rr, depth,
                   /*mode=*/3, /*tp_x=*/0, /*tp_y=*/0, /*clut_x=*/0, /*clut_y=*/0,
                   0, 0, 0, 0, 0, 0, 1023, 511, kRopeBlend);
  }
  // The PRESENT index leads: without it a producer call cannot be tied to the frame it drew into, and
  // an A/B pixel gate has no way to tell "the rope contributed nothing here" from "the producer was
  // never called here" — the two look identical in a bare emission log, and telling them apart is what
  // this channel is for.
  lucent::debug("ropeline", "f{} A=({},{},{}) B=({},{},{}) -> ({:.1f},{:.1f})..({:.1f},{:.1f}) col={},{},{},{}",
                gpu_frame_no(c), ax, ay, az, bx, by, bz, cx[0], cy[0], cx[3], cy[3],
                (int)col[0], (int)col[1], (int)col[2], (int)col[3]);
}

// FUN_8013E08C — the expanding SHOCKWAVE RING. Ported 2026-07-28; it was surfaced by
// PSXPORT_DEBUG=nofx (kanban #65) as a type-0x20 render fn on no whitelist, i.e. an effect nothing
// drew at all.
//
// RE (Ghidra scratch/decomp/fx_e08c.c + raw LWC2/COP2 decoding — the decompile alone was NOT enough,
// see below; re-derived 2026-08-06 against the emitter's own instruction stream in
// generated/ov_a00_shard_0.c `ov_a00_gen_8013E08C`). The guest:
//   * builds a UNIFORM SCALE matrix in scratchpad 0x1F800000, all three CR-packed diagonal slots =
//     (s16)node+0x50 << 4 (1.3.12), off-diagonals cleared;
//   * derives the stroke COLOUR from the SAME field — v = 0x80 - ((node+0x50 - 0x14) * 0x80) / 200 —
//     so the ring fades as it grows, one animator driving both;
//   * composes camera x scale (matMul 0x80084110, which leaves the CAMERA rotation loaded in GTE
//     CR0-4) and loads the product with SetRotMatrix/SetTransMatrix. The translation is built by
//     0x80084220 — an MVMVA of the CURRENT GTE rotation, i.e. the camera, against the SVECTOR AT
//     node+0x2C — plus the camera's own translation at scratchpad 0x1F80010C. Algebraically that is
//     view = camera . (scale . v + nodePos) + camTrans, the ordinary object transform, so natively it
//     is projComposeObjectHost(Robj = diag(scale), Tobj = nodePos). No GTE needed.
//   * walks a FIXED 15-point vertex table at 0x8014C780 (2 words per point: packed VXY then VZ) in 7
//     steps of stride 4 words, RTPT-ing points [2i], [2i+1], [2i+2] — 7 overlapping 3-point spans
//     that cover the whole ring. The table is a circle of RADIUS 256 in the XZ plane sampled every
//     24 degrees ((256,0,0), (233,0,104), ... = 256cos/256sin), which is what makes this a ring.
//   * per span emits TWO monochrome semi-transparent poly-lines (GP0 code 0x4A, 0x55555555
//     terminator), each preceded in the ordering table by its OWN DR_MODE: the unshifted copy under
//     tpage 53 (blend bits = 1, B+F additive) and a copy at (+2,+1) under tpage 85 (blend bits = 2,
//     B-F subtractive), inserted so the subtractive copy draws FIRST. An embossed highlight+shadow
//     pair, not a 2px stroke.
//
// WHERE nodePos COMES FROM, because this producer got it wrong for nine days (claim C036): the world
// position is the packed SVECTOR at node+0x2C (X@0x2C, Y@0x2E, Z@0x30) — the emitter passes node+0x2C
// straight to 0x80084220, which loads word0 as VXY0 and word1 as VZ0. It is NOT node+0x4E: that is the
// ROPE/TETHER node family's layout (kOwnPosX below), and on a ring node 0x50 is the scale animator, so
// reading a position there put the ring's Y equal to its own radius and its X/Z near the world origin,
// thousands of units from the camera. Every span then projected off-screen and the layer drew nothing
// while the guest drew it centre-screen. The narration swirl (game/render/narration_swirl.cpp), the
// other ported type-0x20 node renderer, reads its translation from node+0x2C the same way.
//
// TWO TRAPS worth keeping. Ghidra rendered the vertex loads as opaque `setCopReg(2, <unresolved>, ...)`
// because they are LWC2, not MTC2 — decoding the raw opcodes gives the real mapping (d0/d1 = VXY0/VZ0
// from the cursor, d2/d3 and d4/d5 from cursor+8 and cursor+16). And it renders the tail of the
// SIBLING emitter 0x8002ECD8 as a call to Trig::rsin, which is impossible; trust the instructions.
//
// NOT reproduced, deliberately: the guest biases its ordering-table index by (s16)node+0x32 to lift
// the ring clear of the ground it sits on. The native queue orders by real per-vertex depth (the
// engine owns ordering — see CLAUDE.md), so there is no OT index to bias.
//
// Read-only, no gen body, projects through the native (fps60-lerped) camera — so the ring
// interpolates like the rest of this file's producers.
void Render::shockwaveRingRender(uint32_t node) {
  Core* c = mCore;
  const ShockwaveRingNode ring{ c, node };
  const int scale = ring.scale();
  const unsigned char grey = ringFadeGrey(scale);

  // Robj = diag(scale), Tobj = the node's own world position: see the algebra in the banner above.
  // scale<<4 IS the 1.3.12 matrix element the guest stores (truncated to the s16 slot it stores into),
  // and projComposeObjectHost takes Robj in that same 1.3.12 convention — 4096 is identity. Dividing
  // it down to a plain float here (as this producer first did) shrank every ring to a single point.
  const float s = (float)(int16_t)(scale << 4);
  const float Robj[3][3] = { { s, 0, 0 }, { 0, s, 0 }, { 0, 0, s } };
  const WorldPt at = ring.worldPos();
  const float Tobj[3] = { (float)at.x, (float)at.y, (float)at.z };
  EObjXform xf; projComposeObjectHost(Robj, Tobj, &xf);

  ObjScope objScope(c, node);
  RenderQueue& rq = c->game->activeRq();

  int drawn = 0, rejected = 0;
  float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  for (int step = 0; step < kRingSpans; step++) {
    ProjVtx p[3];
    bool behind = false;
    for (int k = 0; k < 3; k++) {
      const uint32_t vp = kRingTable + (uint32_t)(step * 2 + k) * 8u;   // stride 4 words, 3 points read
      const uint32_t vxy = c->mem_r32(vp);
      xf.project((int16_t)vxy, (int16_t)(vxy >> 16), (int16_t)c->mem_r32(vp + 4), &p[k]);
      if (p[k].sz <= 0) { behind = true; break; }
    }
    if (behind) { rejected++; continue; }    // the guest's own negative-flag reject

    // The embossed pair: subtractive shadow first (it draws underneath), additive highlight over it.
    for (int seg = 0; seg + 1 < 3; seg++)
      strokeSegment(c, rq, p[seg], p[seg + 1], kRingShadowDx, kRingShadowDy, grey, kRingShadowBlend);
    for (int seg = 0; seg + 1 < 3; seg++)
      strokeSegment(c, rq, p[seg], p[seg + 1], 0.0f, 0.0f, grey, kRingHighlightBlend);

    for (int k = 0; k < 3; k++) {
      if (!drawn && !k) { x0 = x1 = p[0].px; y0 = y1 = p[0].py; }
      if (p[k].px < x0) x0 = p[k].px;  if (p[k].px > x1) x1 = p[k].px;
      if (p[k].py < y0) y0 = p[k].py;  if (p[k].py > y1) y1 = p[k].py;
    }
    drawn++;
  }
  // The DENOMINATOR matters more than the emission here: this producer's last "zero pixels" verdict
  // (C036) could not tell a ring that drew nothing from a ring that was never asked to draw. So the
  // line always carries how many of the 7 spans projected, how many the behind-camera test rejected,
  // and the screen box they landed in — a run where the ring is invisible says WHICH of those it was.
  lucent::debug("ropeline",
                "f{} shockwave node={:08X} scale={} grey={} world=({},{},{}) spans={}/{} behind={} "
                "screen=({:.1f},{:.1f})..({:.1f},{:.1f}){}",
                gpu_frame_no(c), node, scale, (int)grey, at.x, at.y, at.z, drawn, kRingSpans, rejected,
                x0, y0, x1, y1,
                drawn ? "" : "  <- NO SPAN PROJECTED: the screen box above is meaningless");
}

// FUN_8013E9D8 — the HANGING object's rope: from the object it hangs off (node+0x14) to itself.
void Render::ropeAnchorRender(uint32_t node) {
  Core* c = mCore;
  const uint32_t anchor = c->mem_r32(node + kAnchorObjPtr);
  if (!anchor) return;
  ObjScope objScope(c, node);
  worldLineDraw((int16_t)c->mem_r16(anchor), (int16_t)c->mem_r16(anchor + 2), (int16_t)c->mem_r16(anchor + 4),
                (int16_t)c->mem_r16(node + kOwnPosX), (int16_t)c->mem_r16(node + kOwnPosX + 2),
                (int16_t)c->mem_r16(node + kOwnPosX + 4));
}

// FUN_8013EA64 — the segmented CHAIN: 8 points the node carries, joined end to end. node+3 says which
// horizontal axis the chain runs along; the other one is fixed for the whole chain.
void Render::ropeChainRender(uint32_t node) {
  Core* c = mCore;
  const uint8_t axis = c->mem_r8(node + kChainAxisFlag);
  if (axis >= 3) return;   // the guest writes no point at all for these states — nothing to draw
  const bool runsInX = (axis == 0);
  const int fixed = (int16_t)c->mem_r16(node + (runsInX ? kChainFixedZ : kChainFixedX));

  ObjScope objScope(c, node);
  WorldPt prev{};
  for (int k = 0; k < kChainPts_N; k++) {
    const int h = (int16_t)c->mem_r16(node + kChainPts + (uint32_t)k * 4u);
    const int y = (int16_t)c->mem_r16(node + kChainPts + 2u + (uint32_t)k * 4u);
    const WorldPt p = runsInX ? WorldPt{ h, y, fixed } : WorldPt{ fixed, y, h };
    if (k) worldLineDraw(prev.x, prev.y, prev.z, p.x, p.y, p.z);
    prev = p;
  }
}

// FUN_80122974 — the TETHER: one rope from this object to an anchor chosen by node+0x47, or (mode 3)
// an eight-segment chain toward the tracked object. Mode 3 is the fishing line.
void Render::tetherLineRender(uint32_t node) {
  Core* c = mCore;
  const WorldPt self{ (int16_t)c->mem_r16(node + kOwnPosX), (int16_t)c->mem_r16(node + kOwnPosX + 2),
                      (int16_t)c->mem_r16(node + kOwnPosX + 4) };
  const uint8_t mode = c->mem_r8(node + kTetherMode);
  ObjScope objScope(c, node);

  if (mode == 0) {
    worldLineDraw(self.x, self.y, self.z, self.x, s16(self.y + kTetherDrop), self.z);
  } else if (mode == 1) {
    worldLineDraw(self.x, self.y, self.z, (int16_t)c->mem_r16(kFixedAnchorX),
                  (int16_t)c->mem_r16(kFixedAnchorY), (int16_t)c->mem_r16(kFixedAnchorZ));
  } else if (mode == 2) {
    worldLineDraw(self.x, self.y, self.z, s16(self.x + (int16_t)c->mem_r16(node + kTetherOffX)),
                  s16((int16_t)c->mem_r16(node + kTetherOffY) + kTetherRise), self.z);
  } else if (mode == 3) {
    const uint32_t tracked = c->mem_r32(kTrackedPtr);
    if (!tracked) return;
    const WorldPt tgt{ (int16_t)c->mem_r16(tracked + kTrackedX), (int16_t)c->mem_r16(tracked + kTrackedY),
                       (int16_t)c->mem_r16(tracked + kTrackedZ) };
    const int dx = tgt.x - self.x, dy = tgt.y - self.y, dz = tgt.z - self.z;
    WorldPt cur = self;
    for (int i = 0; i < kTetherSegs; i++) {
      const int n = i + 1;
      const WorldPt next{ s16(self.x + div8(dx * n)), s16(self.y + div8(dy * n)), s16(self.z + div8(dz * n)) };
      worldLineDraw(cur.x, cur.y, cur.z, next.x, next.y, next.z);
      cur = next;
    }
  }
}
