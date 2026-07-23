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
#include "cfg.h"
#include <math.h>

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
constexpr uint32_t kOwnPosX    = 0x4Eu;   // the node's own world position (X,Y,Z at 0x4E/0x50/0x52)
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
  if (cfg_dbg("ropeline"))
    cfg_logf("ropeline", "A=(%d,%d,%d) B=(%d,%d,%d) -> (%.1f,%.1f)..(%.1f,%.1f) col=%d,%d,%d,%d",
             ax, ay, az, bx, by, bz, (double)cx[0], (double)cy[0], (double)cx[3], (double)cy[3],
             col[0], col[1], col[2], col[3]);
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
