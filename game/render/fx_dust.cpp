// game/render/fx_dust.cpp — NATIVE PRODUCER for Tomba's movement DUST PUFF (kanban #39).
//
// WHAT IT IS. When Tomba starts walking, skids out of a run or turns while running, his motion-state
// dispatcher asks FUN_80053670 for dust; that spawns a controller, and the controller calls
// FUN_80031558 to attach the VISUAL node — a type-0x20 render node with behaviour FUN_80029B40
// (game/ai/beh_pos_history_trail.cpp: a 6-slot ring of the owner's recent positions) and custom render
// fn FUN_80029F6C. Under pc_render nothing drew it: the field walk skipped every type-0x20 fn it had no
// producer for, so the whole effect was invisible. This is that producer.
//
// RE of FUN_80029F6C (ground truth generated/shard_6.c gen_func_80029F6C, and gen_func_80029664 /
// gen_func_80027768 for the two things it draws). `sub` = node+3, the dust SUBTYPE = surface material+1,
// which is what makes a puff on grass differ from a puff on stone. It draws TWO layers:
//
//  1. THE TRAIL (FUN_80029664, always): a tapered additive streak threaded through the first four ring
//     slots. Each consecutive pair of visible points becomes two untextured gouraud quads — one either
//     side of the segment — whose outer edge is black and whose inner edge carries the per-subtype
//     colour ramp at 0x800A1EF0[sub]. The half-width is the per-subtype byte at 0x800A1FA4[sub] turned
//     into a SCREEN-space perpendicular (ratan2 of the segment, +0x400) and scaled by each endpoint's
//     own depth-cue factor, so the streak narrows with distance. Blend mode is the guest's tpage 53 =
//     additive.
//
//  2. THE PUFF (FUN_80027768, gated): the packed mesh at 0x8009F3E0 drawn FOUR times, 0x400 (90 degrees)
//     apart about its own axis, at the recorded position node+0x30/0x32/0x34, yawed by the owner's
//     facing (owner+0x68) and rolled by the puff's age (node+0x36 - 0x400). Its transform is
//     rotmat(0, ownerYaw, age-0x400) then rotY(i*0x400), column-scaled by the per-age bytes at
//     0x800A1FEC[node+6]. High subtypes (sub >= 6) take the sibling path instead: the base matrix is the
//     OWNER's own matrix block (owner+0x98), the second rotation is rotZ, the scale is the fixed triple
//     at 0x800A1CD4, and the mesh is 0x8009F398. The mesh colours are fully replaced by the far colour
//     0x800A1FC8[sub] (the emitter publishes depth-cue IR0 = 0xFFF), and the U scroll (node+0x36 << 5,
//     low byte) walks the puff through its texture frames. The gate is the guest's own: node+5 must be 2
//     or 3 (the ring is populated and not yet retired) and (node+0x36 & 7) < 3.
//
// This is a PORT, not a tap: no gen_func body runs for the picture and no GTE op is used. Everything is
// projected through projComposeObjectHost / projComposeCamera, i.e. the fps60-lerped scene camera, so
// the puff and its streak interpolate with the rest of the frame instead of stepping at the logic rate.
// Read-only — the guest still runs FUN_80029F6C underneath and owns every write (ring shift, animation
// cursor, packet pool, OT).
#include "core.h"
#include "game.h"
#include "game_ctx.h"        // trigOf(c) — the native Trig helpers
#include "render.h"
#include "render_queue.h"
#include "render_internal.h"  // ObjScope / proj_pz_to_ord
#include "mesh_quads.h"
#include "effect_lerp.h"
#include "trig.h"
#include "cfg.h"

namespace {

// --- node field layout (RE'd from gen_func_80029F6C / FUN_80031558) --------------------------------
constexpr uint32_t kSubtype   = 0x03u;   // u8 dust subtype = surface material + 1
constexpr uint32_t kState     = 0x05u;   // u8 ring state; the puff draws only in states 2 and 3
constexpr uint32_t kAgeIdx    = 0x06u;   // u8 index into the per-age column-scale table
constexpr uint32_t kOwner     = 0x10u;   // the object the dust came off (Tomba)
constexpr uint32_t kPosX      = 0x30u;   // s16 recorded world position (x, y, z at +0x30/+0x32/+0x34)
constexpr uint32_t kAge       = 0x36u;   // u16 age: low 3 bits = frame counter, rest = owner phase
constexpr uint32_t kRing      = 0x38u;   // 6-slot position ring, stride 8: packed VX,VY then VZ
constexpr uint32_t kOwnerYaw  = 0x68u;   // s16 facing of the owner object

// --- guest data tables ----------------------------------------------------------------------------
constexpr uint32_t kTrailColour = 0x800A1EF0u;   // + sub*20: 5 colour words, the streak's ramp
constexpr uint32_t kTrailWidth  = 0x800A1FA4u;   // + sub*4 : 4 half-width bytes, one per ring point
constexpr uint32_t kFarColour   = 0x800A1FC8u;   // + sub*4 : 3 bytes, the puff tint (<<4 = far colour)
constexpr uint32_t kAgeScale    = 0x800A1FECu;   // + node[6]*4 : 3 column-scale bytes (<<2)
constexpr uint32_t kFixedScale  = 0x800A1CD4u;   // the sub>=6 path's fixed column scale
constexpr uint32_t kMeshLow     = 0x8009F3E0u;   // puff mesh, subtypes 1..5
constexpr uint32_t kMeshHigh    = 0x8009F398u;   // puff mesh, subtypes 6..10
constexpr uint32_t kOwnerMatrix = 0x98u;         // the sub>=6 path's base matrix, on the owner
constexpr int      kSubMeshSplit = 6;            // node[3] < 6 takes the rotmat/rotY path
constexpr int      kQuarterTurn  = 0x400;        // 1024 = 90 degrees in the engine's 4096-per-turn angles
constexpr int32_t  kCueFull      = 0xFFF;        // IR0 the emitter publishes: colour := far colour
constexpr int      kTrailBlend   = 1;            // guest tpage 53 -> abr 1 = additive
constexpr int      kRingPoints   = 4;            // FUN_80029664's count argument
constexpr int      kPuffPoint    = 4;            // the puff origin, carried alongside the ring points

// One trail point: where it landed on screen, how deep, and its depth-cue scale (which is what turns the
// authored half-width into pixels).
struct TrailPoint {
  bool  valid;
  float x, y, ord;
  int32_t cueScale;
};

// The emitters' shared depth-cue-as-scale relation: with DQA = 6 / DQB = 0, RTPS leaves
// MAC0 = 6·n, n = (H·0x20000/SZ3 + 1)/2.
int32_t cueScaleOf(uint32_t H, int sz) {
  if (sz <= 0) return 0;
  int64_t div = ((int64_t)H << 17) / sz;
  if (div > 0x1FFFF) div = 0x1FFFF;
  return (int32_t)(((div + 1) >> 1) * 6);
}

// FUN_80029664's per-point visibility gate: the OT bucket the point quantizes into must land in
// [4, 0x7FF]. (This variant quantizes the raw SZ3, hence the >>12 rather than the >>10 the sprite
// emitters use on an already-halved key.)
bool trailPointVisible(int sz) {
  if (sz <= 0) return false;
  const int32_t shift = sz >> 12;
  const int32_t k = (sz >> 2 >> (shift & 31)) + (shift << 9);
  return (uint32_t)(k - 4) < 2044u;
}

}  // namespace

// FUN_80029664 — the trail: thread the ring's first four recorded positions and lay two additive gouraud quads along
// each segment. Screen-space perpendicular exactly as the guest builds it, but in float and through the
// native camera, so the streak lerps.
int Render::dustTrailEmit(const EffectPoints& pts, const EObjXform& cam, int sub) {
  Core* c = mCore;
  const uint32_t colours = kTrailColour + (uint32_t)sub * 20u;
  const uint32_t widths  = kTrailWidth  + (uint32_t)sub * 4u;
  const uint32_t H = (uint32_t)cam.H;
  const int offX = c->game->gpu.s_off_x, offY = c->game->gpu.s_off_y;

  TrailPoint prev = {};
  int segments = 0;
  float perpPrevX = 0.0f, perpPrevY = 0.0f;
  for (int i = 0; i < kRingPoints; i++) {
    TrailPoint cur = {};
    if (pts.valid[i]) {
      ProjVtx pv; cam.project(pts.x[i], pts.y[i], pts.z[i], &pv);
      if (trailPointVisible(pv.sz)) {
        cur.valid = true; cur.x = pv.px; cur.y = pv.py; cur.ord = proj_pz_to_ord(pv.pz);
        cur.cueScale = cueScaleOf(H, pv.sz);
      }
    }

    if (i > 0 && cur.valid && prev.valid) {
      // Perpendicular: the segment's screen angle turned a quarter turn, at the authored half-width,
      // scaled into pixels per endpoint by that endpoint's own depth-cue factor.
      const int32_t seg = trigOf(c).ratan2((int32_t)(cur.y - prev.y), (int32_t)(cur.x - prev.x))
                          + kQuarterTurn;
      const int32_t w = (int32_t)c->mem_r8(widths + (uint32_t)i);
      const int32_t ux = (w * trigOf(c).rcos(seg) + 2048) >> 12;
      const int32_t uy = (w * trigOf(c).rsin(seg) + 2048) >> 12;
      const float curPx = (float)(int32_t)(((int64_t)ux * cur.cueScale) >> 16);
      const float curPy = (float)(int32_t)(((int64_t)uy * cur.cueScale) >> 16);
      if (i == 1) {   // the first segment seeds the near end's perpendicular off the previous point
        perpPrevX = (float)(int32_t)(((int64_t)ux * prev.cueScale) >> 16);
        perpPrevY = (float)(int32_t)(((int64_t)uy * prev.cueScale) >> 16);
      }

      const uint32_t cIn  = c->mem_r32(colours + (uint32_t)(i - 1) * 4u);
      const uint32_t cOut = c->mem_r32(colours + (uint32_t)i * 4u);
      const float depth[4] = { prev.ord, cur.ord, prev.ord, cur.ord };
      // Two quads, one per side: outer edge black, inner edge (the segment itself) the ramp colour.
      for (int side = 0; side < 2; side++) {
        const float sgn = side ? 1.0f : -1.0f;
        const float pxf[4] = { prev.x + sgn * perpPrevX, cur.x + sgn * curPx, prev.x, cur.x };
        const float pyf[4] = { prev.y + sgn * perpPrevY, cur.y + sgn * curPy, prev.y, cur.y };
        int xs[4], ys[4]; float xsf[4], ysf[4];
        for (int k = 0; k < 4; k++) {
          xsf[k] = pxf[k] + (float)offX; ysf[k] = pyf[k] + (float)offY;
          xs[k] = (int)(xsf[k] < 0 ? xsf[k] - 0.5f : xsf[k] + 0.5f);
          ys[k] = (int)(ysf[k] < 0 ? ysf[k] - 0.5f : ysf[k] + 0.5f);
        }
        const unsigned char rr[4] = { 0, 0, (unsigned char)(cIn & 0xFFu),         (unsigned char)(cOut & 0xFFu) };
        const unsigned char gg[4] = { 0, 0, (unsigned char)((cIn >> 8) & 0xFFu),  (unsigned char)((cOut >> 8) & 0xFFu) };
        const unsigned char bb[4] = { 0, 0, (unsigned char)((cIn >> 16) & 0xFFu), (unsigned char)((cOut >> 16) & 0xFFu) };
        const int us[4] = { 0, 0, 0, 0 }, vs[4] = { 0, 0, 0, 0 };
        c->game->activeRq().emitOrQueue(c, /*capture=*/1, RQ_WORLD, RQ_OM_DEPTH, /*nv=*/4, /*semi=*/1,
                                        /*raw=*/0, xs, ys, xsf, ysf, us, vs, rr, gg, bb, depth,
                                        /*mode=*/3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1023, 511, kTrailBlend);
        if (cfg_dbg("fxdust"))
          cfg_logf("fxdust", "  trail seg=%d side=%d v0=(%d,%d) v1=(%d,%d) v2=(%d,%d) v3=(%d,%d)",
                   i, side, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], xs[3], ys[3]);
      }
      perpPrevX = curPx; perpPrevY = curPy;
      segments++;
    }
    prev = cur;
  }
  return segments;
}

// The puff layer: four copies of the subtype's mesh, a quarter turn apart, tinted by the surface material.
void Render::dustPuffEmit(uint32_t node, const EffectPoints& pts, int sub) {
  Core* c = mCore;
  const uint32_t owner = c->mem_r32(node + kOwner);
  const uint16_t age = c->mem_r16(node + kAge);
  const int16_t roll = (int16_t)(age - (uint16_t)kQuarterTurn);
  const int uBias = (int)(uint8_t)(uint16_t)(age << 5);       // texture frame walk
  const bool lowSub = ((int)c->mem_r8(node + kSubtype) < kSubMeshSplit);

  const uint32_t scaleSrc = lowSub ? (kAgeScale + (uint32_t)c->mem_r8(node + kAgeIdx) * 4u) : kFixedScale;
  const int32_t colScale[3] = { (int32_t)c->mem_r8(scaleSrc + 0u) << 2,
                                (int32_t)c->mem_r8(scaleSrc + 1u) << 2,
                                (int32_t)c->mem_r8(scaleSrc + 2u) << 2 };
  const uint32_t fc = kFarColour + (uint32_t)sub * 4u;
  const int32_t farColour[3] = { (int32_t)c->mem_r8(fc + 0u) << 4,
                                 (int32_t)c->mem_r8(fc + 1u) << 4,
                                 (int32_t)c->mem_r8(fc + 2u) << 4 };
  const float Tobj[3] = { (float)pts.x[kPuffPoint], (float)pts.y[kPuffPoint], (float)pts.z[kPuffPoint] };

  int32_t base[3][3];
  if (lowSub) MeshQuads::rotmat(c, 0, c->mem_r16s(owner + kOwnerYaw), roll, base);
  else        MeshQuads::fromGuest(c, owner + kOwnerMatrix, base);

  for (int i = 0; i < 4; i++) {
    const int16_t spin = (int16_t)(i * kQuarterTurn);
    int32_t axis[3][3];
    if (lowSub) MeshQuads::rotY(c, spin, axis);
    else        MeshQuads::rotZ(c, spin, axis);
    float Robj[3][3];
    MeshQuads::composeScaled(base, axis, colScale, Robj);
    EObjXform w; projComposeObjectHost(Robj, Tobj, &w);
    projSetActive(&w);
    meshQuadRecordsEmit(lowSub ? kMeshLow : kMeshHigh, uBias, farColour, kCueFull);
    projClearActive();
  }
}

// FUN_80029F6C — the dust node's custom render fn, rebuilt natively.
void Render::dustEffectRender(uint32_t node) {
  Core* c = mCore;
  const int sub = (int)c->mem_r8(node + kSubtype);
  ObjScope objScope(c, node);   // prim identity (dbg_node) = the dust node itself

  // The effect's own world state, through the actor-transform interpolation tier: on the fps60
  // in-between present this comes back blended with last frame's, so the trail and the puff MOVE
  // between the two real frames instead of holding. Same producer, same code path, both frames.
  EffectPoints live;
  live.n = kRingPoints + 1;
  for (int i = 0; i < kRingPoints; i++) {
    const uint32_t slot = node + kRing + (uint32_t)i * 8u;
    const uint32_t p0 = c->mem_r32(slot), p1 = c->mem_r32(slot + 4u);
    live.valid[i] = (p0 != 0);
    live.x[i] = (int16_t)p0; live.y[i] = (int16_t)(p0 >> 16); live.z[i] = (int16_t)p1;
  }
  live.valid[kPuffPoint] = true;
  live.x[kPuffPoint] = c->mem_r16s(node + kPosX);
  live.y[kPuffPoint] = c->mem_r16s(node + kPosX + 2u);
  live.z[kPuffPoint] = c->mem_r16s(node + kPosX + 4u);
  const EffectPoints& pts = mEffectLerp.resolve(c, node, live);

  EObjXform cam; projComposeCamera(&cam);
  const int segments = dustTrailEmit(pts, cam, sub);

  // The guest's own gate on the puff layer: the ring must be live (state 2 or 3) and the puff not yet
  // aged out of its three frames. The trail above draws either way — the guest emits it unconditionally.
  const uint8_t state = c->mem_r8(node + kState);
  const uint16_t age = c->mem_r16(node + kAge);
  const bool puff = (state == 2 || state == 3) && (age & 7u) < 3u;
  if (puff) dustPuffEmit(node, pts, sub);

  if (cfg_dbg("fxdust"))
    cfg_logf("fxdust", "f%d t=%.2f node=%08X sub=%d state=%d age=%04X ageIdx=%d owner=%08X "
                       "pos=(%d,%d,%d) segments=%d puff=%d",
             c->game->gpu.s_frame, (double)c->game->fps60.mT, node, sub, (int)state, (unsigned)age, (int)c->mem_r8(node + kAgeIdx),
             c->mem_r32(node + kOwner), c->mem_r16s(node + kPosX), c->mem_r16s(node + kPosX + 2u),
             c->mem_r16s(node + kPosX + 4u), segments, (int)puff);
}
