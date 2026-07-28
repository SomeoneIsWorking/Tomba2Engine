// game/render/fx_sprite.cpp — NATIVE PRODUCERS for the WORLD-ANCHORED SCALED-SPRITE effect families.
// Two of them live here because they share the whole anchoring/scaling contract and differ only in how
// one frame's quads are packed:
//   Render::fxSpriteRender      — the FUN_80027A4C family (8-byte records): torch / hut-roof flames
//                                 (kanban #12 restored the layer, #23 makes it LERP at fps60).
//   Render::fxAnimSpriteRender  — the FUN_8002847C family (36-byte four-corner records, animation-script
//                                 driven): Tomba's movement DUST PUFFS + the impact starburst (kanban #39).
//
// WAS a leaf tap on 0x80027A4C (run the gen body, scrape the scratchpad the emitter published, re-derive
// the quads host-side). A tap fires at GUEST time on the real frame only and re-uses the guest's own
// RTPS result, so its output can never interpolate — the interp present re-run would have to re-execute
// the gen body under a lerped camera, which writes guest RAM. That is exactly #23: the roof flames step
// at 30 Hz. The fix (per CLAUDE.md "LERP IS NATIVE TOO" / "NATIVE PRESENTATION") is a real port: PROJECT
// the effect's own world state with the native camera and emit from the display pass, so tier1Render
// re-renders it one frame behind under the lerped camera like every other native producer.
//
// RE (Ghidra scratch/decomp/fx_emit_leaves.c + fx_pkt_writer.c, ground truth generated/shard_5..7.c). The
// family has ONE packet writer (0x80027A4C) and THREE emitter render-fns; a type-0x20 render node carries
// its emitter at node+0x18 (confirmed live at the seaside water-pump vista, HEAD 0x800F2624: six
// FUN_80027CB4 roof-flame nodes + eight FUN_800281EC particle-flame nodes, all vis=1). Every emitter:
//   1. loads the pure scene-camera CRs (scratchpad 0x1F8000F8..0x114) into the GTE and sets DQA=6 (or 4
//      for the FUN_800281EC '!' variant) / DQB=0 — i.e. it REPURPOSES the GTE's depth-cue divide as a
//      per-Z sprite scale: after RTPS, MAC0 (GTE data reg 0x18) = n·DQA with n=(H·0x20000/SZ3 + 1)/2.
//   2. RTPS the WORLD anchor (node+0x2C packed VX,VY / node+0x30 VZ): SXY2 = screen anchor, SZ3 = depth,
//      MAC0 = base pixel scale.
//   3. range-checks the OT bucket key quantize((SZ3>>2)+node+0x32); out of range -> skip (no emit).
//   4. FUN_80027A4C(a0 = 8-byte record list node+0x34, a1 = clut|tpage node+0x44) walks the records.
// Scale per emitter:
//   FUN_80027CB4  scaleX = MAC0                                (uniform)
//   FUN_80027E5C  scaleX = MAC0 * (u8)node+0x6  >> 4
//   FUN_800281EC  PER-PARTICLE: particles at node+0x50 stride 8, count (s16)node+0x4E; each = anchor
//                 (p[0] packed VX,VY / p[1] VZ) RTPS'd individually, scaleX = MAC0 * (s16)p+0x6 >> 8.
// The native camera projection (projComposeCamera -> EObjXform::project) reproduces the GTE RTPS in float
// (docs tomba2-native-projection, 0-diff vs Beetle) and — crucially — reads the camera through the
// Fps60::sceneCam choke, which returns the LERPED camera at the interp present. So projecting the anchor
// natively gives a screen position that MOVES SMOOTHLY as the camera pans: the flame lerps for free. The
// base scale is derived from the same native SZ3 + the emitter's own DQA constant (no ambient GTE read),
// so it too tracks the lerped depth. The 8-byte record walk (corners = anchor + s8-offset·scale>>16, UV
// flip rules, flat-grey brightness) is unchanged from the tap — only the anchor/scale SOURCE changed from
// "scrape the gen body's scratchpad" to "project the world anchor natively".
//
// 0x80027A4C now runs its plain gen body for guest-state fidelity (SBS stays byte-exact — the guest still
// executes it; pc_render simply no longer scrapes it). Read-only overlay: no guest write.
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"
#include "mesh_quads.h"      // host-side Math::rotmat / trig for the rotated ring member
#include "render_internal.h"   // ObjScope / cur_render_node / proj_pz_to_ord
#include "effect_lerp.h"
#include "fx_sprite.h"         // SpriteAnchor — the family's shared scale / OT-gate / depth-cue relations
#include "game_ctx.h"          // trigOf(c)
#include "trig.h"              // Trig::rsin / rcos — the ports of FUN_80083E80 / FUN_80083F50
#include "cfg.h"

// --- the family's shared relations (fx_sprite.h) ---------------------------------------------------
// The emitters set DQA to a small integer and DQB to 0 before RTPS, so the depth-cue MAC0 becomes a
// per-Z sprite scale. baseScale reproduces MAC0 = n·DQA, n=(H·0x20000/SZ3 + 1)/2 (the RTPS perspective
// divide, integer-faithful; the GTE's UNR reciprocal differs by <1 ULP and never changes a pixel here).
int32_t SpriteAnchor::baseScale(uint32_t H, int sz, int dqa) {
  int64_t div = ((int64_t)H << 17) / sz;          // H * 0x20000 / SZ3
  if (div > 0x1FFFF) div = 0x1FFFF;               // GTE divide saturates (also the SZ3-too-small case)
  const int64_t n = (div + 1) >> 1;
  return (int32_t)(n * dqa);
}

// The emitter's OT-key range gate (its emit/skip decision), reproduced on the native SZ3 so a producer
// draws exactly the anchors the guest would. Logarithmic bucket map, valid range [4, 0x7FF].
bool SpriteAnchor::otKeyInRange(int sz, int bias) {
  if (sz <= 0) return false;
  int k = (sz >> 2) + bias;
  if (k < 4) k = 4;
  k = (k >> ((k >> 10) & 0x1f)) + (k >> 10) * 0x200;
  return (uint32_t)(k - 4) <= 0x7FBu;
}

// GTE DPCS with sf=1, lm=0 (the packet writer's copFunction(2,0x780010)):
//   MAC = comp<<16 ; IR = ((FC<<12) - MAC) >> 12 ; IR = (IR*IR0 + MAC) >> 12 ; out = IR/16 saturated.
uint8_t SpriteAnchor::depthCue(uint8_t comp, int32_t ir0, int32_t farComp) {
  const int64_t mac = (int64_t)comp << 16;
  int64_t ir = (((int64_t)farComp << 12) - mac) >> 12;
  ir = ((ir * ir0) + mac) >> 12;
  if (ir > 32767) ir = 32767;
  if (ir < -32768) ir = -32768;
  int64_t out = ir / 16;
  if (out < 0) out = 0;
  if (out > 255) out = 255;
  return (uint8_t)out;
}

namespace {

// --- node field layout (RE'd; confirmed live) ------------------------------------------------------
constexpr uint32_t kAnchorXY = 0x2Cu;   // packed VX (lo16) | VY (hi16), world
constexpr uint32_t kAnchorZ  = 0x30u;   // VZ in lo16, world
constexpr uint32_t kOtBias   = 0x32u;   // s16 OT-key bias
constexpr uint32_t kRecList  = 0x34u;   // 8-byte record list (a0 to the packet writer)
constexpr uint32_t kClutPage = 0x44u;   // clut (lo16) | tpage (hi16) (a1)
constexpr uint32_t kRenderFn = 0x18u;   // custom render fn = the emitter address
constexpr uint32_t kE5cScale = 0x06u;   // u8 extra scale numerator (FUN_80027E5C)
constexpr uint32_t kVariant3 = 0x03u;   // '!' selects DQA=4 in the FUN_800281EC variant
constexpr uint32_t kPartCount= 0x4Eu;   // s16 particle count (FUN_800281EC)
constexpr uint32_t kPartArray= 0x50u;   // particle array base, stride 8 (FUN_800281EC)
constexpr uint32_t kPartScale= 0x06u;   // s16 per-particle scale numerator (particle+6)

constexpr uint32_t FN_UNIFORM   = 0x80027CB4u;   // scaleX = MAC0
constexpr uint32_t FN_BYTESCALE = 0x80027E5Cu;   // scaleX = MAC0 * node[6] >> 4
constexpr uint32_t FN_PARTICLE  = 0x800281ECu;   // per-particle loop
// FOURTH member of the family, found 2026-07-28 by PSXPORT_DEBUG=nofx (kanban #65) — it reaches the
// same writer 0x80027A4C but was on no whitelist, so its effect drew NOTHING at all (the tap this
// producer replaced is gone, so an un-whitelisted emitter has no other path to a picture).
// RE (scratch/decomp/fx_801c.c): identical shape to the other three — pure scene-camera CRs, DQA=6,
// DQB=0, RTPS the world anchor at node+0x2C/+0x30, the same (SZ3>>2)+node+0x32 OT-key range gate,
// then FUN_80027A4C(node+0x34, node+0x44). What is NEW is the scale: it is the first emitter with a
// SEPARATE X and Y multiplier — scaleX = MAC0*(s16)node+0x48 >> 8 and scaleY = MAC0*(s16)node+0x4A
// >> 8 (the guest writes both 0x1F800084 and 0x1F800088; the other three write only the X slot).
constexpr uint32_t FN_XYSCALE   = 0x8002801Cu;   // scaleX/Y = MAC0 * node+0x48 / node+0x4A >> 8
// FIFTH member, ported 2026-07-28 (kanban #65 / portmap fx-sprite-emitter-b3a4). It reaches the same
// writer 0x80027A4C but does NOT fit the scale-rule switch, because it is the only one that composes
// a PER-NODE ROTATION instead of loading the pure scene camera: Math::rotmat from node+0x48, three
// MVMVA column composes, then the node position through MVMVA + the camera translation. That is
// exactly projComposeObjectHost(rotation, position), so it gets its own branch below.
// It places FOUR sprites on a horizontal ring in the node's own rotated frame — the sweep is
// `angle = loopCounter << 10` (verified from `sll r16, r18, 10` at 0x8002B620 with `addu r4, r16, r0`
// in the delay slot; bound `slti r2, r18, 4` at 0x8002B770), i.e. 0/1024/2048/3072 over a 0..0xFFF
// domain = the four cardinal directions. Radius comes from (trig * 0x19) >> 4 and the height is the
// constant (s16)node+0x50 << 6.
constexpr uint32_t FN_RINGROT   = 0x8002B3A4u;
constexpr uint32_t kRingAngles  = 0x48u;   // 3 x s16 Euler angles for the node's own rotation
constexpr uint32_t kRingHeight  = 0x50u;   // s16; << 6 gives the ring's constant Y
constexpr int      kRingPoints  = 4;       // slti r18, 4
constexpr int      kRingRadiusN = 0x19;    // (trig * 0x19) >> 4
constexpr uint32_t kXScaleMul   = 0x48u;
constexpr uint32_t kYScaleMul   = 0x4Au;

}  // namespace

// Walk the 8-byte quad records at rec0 and draw each as a textured world quad, sized by (scaleX,scaleY)
// about the native-projected float anchor (anchorXf,anchorYf) at view depth `od`. Corners are kept in
// float (sub-pixel) so the quad interpolates smoothly, and the draw goes through
// RenderQueue::drawWorldQuad (has_xyf=1 -> tier1-owned -> re-rendered under the lerped camera).
// ir0/farColour reproduce the packet writer's DPCS on each record colour — 0 for the flame emitters
// (identity cue), driven for the portal. Read-only.
void Render::spriteRecordsEmit(uint32_t rec0, uint32_t clutPage,
                               float anchorXf, float anchorYf, float od,
                               int32_t scaleX, int32_t scaleY, int32_t ir0,
                               const int32_t* farColour) {
  Core* c = mCore;
  const float sxf = (float)scaleX / 65536.0f, syf = (float)scaleY / 65536.0f;
  const float depth[4] = { od, od, od, od };
  uint32_t rec = rec0;
  for (int guard = 0; guard < 256; guard++) {
    const uint32_t w0 = c->mem_r32(rec);
    const uint32_t w1 = c->mem_r32(rec + 4u);
    const uint32_t flags = w1 >> 16;

    // Corners: float anchor + signed-byte offset * scale (the guest's (s8*scale)>>16, kept in float).
    const float xL = anchorXf + (float)(int8_t)(w0)       * sxf;
    const float xR = anchorXf + (float)(int8_t)(w0 >> 8)  * sxf;
    const float yT = anchorYf + (float)(int8_t)(w0 >> 16) * syf;
    const float yB = anchorYf + (float)(int8_t)(w0 >> 24) * syf;

    // UVs with the guest's flip rules (note the asymmetric 7/6 insets — mirrored exactly).
    const uint32_t u = w1 & 0xFFu, v = (w1 >> 8) & 0xFFu;
    const int ub = (int)(u & 0xF8u), uw = (int)(u & 7u) * 8;
    const int vb = (int)(v & 0xF8u), vh = (int)(v & 7u) * 8;
    int u02, u13, v01, v23;
    if (!(flags & 0x2000u)) { u02 = ub + 1;      u13 = ub + uw + 7; }
    else                    { u02 = ub + uw + 6; u13 = ub + 1;      }
    if (!(flags & 0x1000u)) { v01 = vb + 1;      v23 = vb + vh + 7; }
    else                    { v01 = vb + vh + 7; v23 = vb + 1;      }

    // Flat grey brightness, run through the writer's DPCS depth cue (identity when ir0 == 0, which is
    // what the flame emitters program; the portal drives it toward its far colour).
    const unsigned char lvl = (unsigned char)(flags & 0xFFu);
    const int semi = (int)(flags & 1u);
    const uint16_t clut  = (uint16_t)((clutPage & 0xFFFFu) + ((flags & 0xF00u) >> 2));
    const uint16_t tpage = (uint16_t)((clutPage >> 16) | ((flags & 6u) << 4));

    float px[4] = { xL, xR, xL, xR };
    float py[4] = { yT, yT, yB, yB };
    int   us[4] = { u02, u13, u02, u13 };
    int   vs[4] = { v01, v01, v23, v23 };
    const unsigned char cr = ir0 ? SpriteAnchor::depthCue(lvl, ir0, farColour ? farColour[0] : 0) : lvl;
    const unsigned char cg = ir0 ? SpriteAnchor::depthCue(lvl, ir0, farColour ? farColour[1] : 0) : lvl;
    const unsigned char cb = ir0 ? SpriteAnchor::depthCue(lvl, ir0, farColour ? farColour[2] : 0) : lvl;
    unsigned char rr[4] = { cr, cr, cr, cr }, gg[4] = { cg, cg, cg, cg }, bb[4] = { cb, cb, cb, cb };
    c->game->activeRq().drawWorldQuad(c, px, py, depth, us, vs, rr, gg, bb, tpage, clut, semi, nullptr);

    rec += 8u;
    if (flags & 0xC000u) break;   // the terminal record IS drawn (gen's post-condition), then stop
  }
}

namespace {

// ===================================================================================================
// THE ANIMATED FOUR-CORNER QUAD FAMILY — emitter FUN_800286CC, packet writer FUN_8002847C.
//
// RE (ground truth generated/shard_4.c gen_func_800286CC + shard_3.c gen_func_8002847C; live node
// 0x800EE730 at seesaw-weight f748-766, type 0x20, node+0x18 = 0x800286CC, behaviour 0x800330AC).
// The emitter is the SAME shape as the flame emitters above — pure scene camera into the GTE, DQA=6 /
// DQB=0 so the depth-cue MAC0 becomes a per-Z pixel scale, RTPS of the node's own world anchor
// (node+0x2C packed VX,VY / node+0x30 VZ), the same OT-bucket range gate on (SZ3>>2)+node+0x32 — with
// two differences:
//   * the pixel scale is PER AXIS: node+0x34 packs two 8.8 multipliers, scaleX = MAC0·(u16)lo >> 8 and
//     scaleY = MAC0·(u16)hi >> 8 (the dust puff runs 0x0100/0x0100 = 1.0, the starburst breathes).
//   * the quad list is chosen by an ANIMATION SCRIPT: node+0x3C points at the current script byte, its
//     low 7 bits index the record-list table at node+0x50 (bit 7 = loop marker, which only tells the
//     guest to rewind its node+0x40 cursor — a guest-state concern this producer does not share).
// One record is 36 bytes and describes a genuine four-corner quad (not an axis-aligned box):
//   +0  uv0 word: u0 | v0<<8 | clut<<16          +12 RGB0        +28 s8 x0, x1  (bytes 28,29)
//   +4  uv1 word: u1 | v1<<8 | tpage<<16, and    +16 RGB1        +30 s8 y0, y1  (bytes 30,31)
//       bit31 clear = another record follows     +20 RGB2        +32 s8 x2, x3  (bytes 32,33)
//   +8  uv2 word, uv3 = (s32)uv2 >> 16           +24 RGB3        +34 s8 y2, y3  (bytes 34,35)
// The guest runs the four colours through DPCT/DPCS, but the emitter first forces the depth-cue factor
// IR0 (scratchpad 0x1F800090) to 0, at which the cue is the identity — so the record colours pass
// through unchanged and no far-colour read is needed here. The GP0 op is a fixed 0x3E: every quad in
// this family is semi-transparent.
constexpr uint32_t kAnimScale  = 0x34u;   // packed 8.8 scale pair: scaleY (hi16) | scaleX (lo16)
constexpr uint32_t kAnimScript = 0x3Cu;   // -> current animation-script byte
constexpr uint32_t kAnimTable  = 0x50u;   // table of per-animation-frame record lists
constexpr uint32_t kAnimStride = 36u;     // one four-corner record
constexpr uint32_t kAnimDqa    = 6u;      // the depth-cue constant this emitter programs

// ── The FUN_800328EC family: the SAME four-corner writer, on a different node layout ─────────────
// FUN_800328EC is not a writer at all — it is three instructions: zero the depth-cue IR0 at
// 0x1F800090 and tail into FUN_8002847C(model, 0, 0), i.e. the very writer fxAnimSpriteRender above
// already reproduces. What differs is the NODE LAYOUT its controllers carry, and the fact that none
// of them was dispatched, so nothing reached the picture.
//
// The family's two shared leaves are worth naming because every controller calls them in order:
//   FUN_800329E0(dqa) — load the pure scene-camera CRs from scratchpad 0x1F8000F8 into GTE CR0-7 and
//                       set DQA=dqa, DQB=0. The same depth-cue-as-scale trick as the 80027A4C family.
//   FUN_800317CC(bias) — RTPS the anchor already in GTE data 0/1, run the OT-key gate on
//                       (SZ3>>2)+bias (identical to SpriteAnchor::otKeyInRange), and on success
//                       publish OT key -> 0x1F800080, SXY2 -> 0x1F80008C, MAC0 -> 0x1F800084.
//                       Returns 0 on emit, 1 on skip. This is the same scratchpad contract
//                       game/render/fx_ring.cpp documents for FUN_8002ECD8 — one publisher, and
//                       FUN_8002ECD8 simply inlines it.
// The controller then scales the published MAC0 into 0x1F800084/0x1F800088 and calls the wrapper.
constexpr uint32_t kAltAnchorX = 0x2Eu;   // three SEPARATE s16s — not the packed pair at 0x2C
constexpr uint32_t kAltAnchorY = 0x32u;
constexpr uint32_t kAltAnchorZ = 0x36u;
constexpr uint32_t kAltScale   = 0x60u;   // packed 8.8 pair: scaleY (hi16) | scaleX (lo16)
constexpr uint32_t kAltScript  = 0x64u;   // -> current animation-script byte (bit 7 = loop marker)
constexpr uint32_t kAltTable   = 0x6Cu;   // table of per-animation-frame record lists

// FUN_8013D454's sprite branch. The controller has two modes on (s16)node+0x60: non-zero draws the
// water jet's MESH through FUN_80027768 (owned by fx_mesh.cpp's scope), zero draws this sprite.
constexpr uint32_t kJetMode     = 0x60u;        // (s16) 0 selects the sprite branch
constexpr uint32_t kJetScale    = 0x62u;        // u16 uniform scale numerator, applied >> 8
constexpr uint32_t kJetModelTab = 0x8010A058u;  // 6 record-list pointers; the sprite branch takes [0]
constexpr int      kJetDqa      = 4;            // this one programs 4, not the family's usual 6
constexpr int      kJetBias     = -64;

// FUN_8012D9E8's sprite tail (see the producer for why each of these is a field, not a constant).
constexpr uint32_t kRotTailAnchor    = 0x60u;        // packed VX|VY here, VZ at +0x64
constexpr uint32_t kRotTailScale     = 0x70u;        // s16 uniform scale numerator, applied >> 11
constexpr uint32_t kRotTailIndex     = 0x40u;        // model index = the SIGNED HIGH BYTE of this s16
constexpr uint32_t kRotTailTableA    = 0x801387A8u;  // chosen when node+3 == 8
constexpr uint32_t kRotTailTableB    = 0x80138790u;
constexpr int      kRotTailTableN    = 6;            // both tables are six record-list pointers
constexpr uint8_t  kRotTailAltSel    = 8u;
constexpr int      kRotTailShift     = 11;
constexpr int      kRotTailDepthBias = -100;

// FUN_80113768's fields (the depth-cued member; see the producer for the RE).
constexpr uint32_t kCuedRecList  = 0x34u;   // same slot as the family's kRecList, reached via 0x800328BC
constexpr uint32_t kCuedScaleX   = 0x48u;   // s16 multipliers, applied to MAC0 >> 8
constexpr uint32_t kCuedScaleY   = 0x4Au;
constexpr uint32_t kCuedCueByte  = 0x07u;   // u8 -> IR0 = byte << 5, the depth-cue strength
constexpr int      kCuedCueShift = 5;

// Walk the 36-byte records at rec0, sizing each corner about the native-projected float anchor. Same
// float-corner / drawWorldQuad treatment as emitSpriteRecords (has_xyf = 1 -> tier1-owned -> re-drawn
// under the lerped camera at the interp present). Read-only.
// ir0 / farColour: the writer runs every record colour through the GTE depth cue with IR0 =
// *(0x1F800090) and the emitter's far colour, exactly as spriteRecordsEmit does for the 8-byte format.
// Most emitters in this family force IR0 = 0, at which the cue is the identity and the record colours
// pass through — so those callers omit both arguments. FUN_8010C7F4's particle field is the one that
// DRIVES it, feeding a per-particle distance so the field fades to black with range.
int emitAnimQuadRecords(Core* c, uint32_t rec0, float anchorXf, float anchorYf, float od,
                        int32_t scaleX, int32_t scaleY,
                        int32_t ir0 = 0, const int32_t* farColour = nullptr) {
  const float sxf = (float)scaleX / 65536.0f, syf = (float)scaleY / 65536.0f;
  const float depth[4] = { od, od, od, od };
  uint32_t rec = rec0;
  int drawn = 0;
  for (int guard = 0; guard < 256; guard++, rec += kAnimStride) {
    const uint32_t uv0 = c->mem_r32(rec + 0u);
    const uint32_t uv1 = c->mem_r32(rec + 4u);       // bit31 = "this is the last record" (still drawn)
    const uint32_t uv2 = c->mem_r32(rec + 8u);
    const uint32_t uv3 = (uint32_t)((int32_t)uv2 >> 16);

    auto sb = [&](uint32_t off) { return (float)(int8_t)c->mem_r8(rec + off); };
    float px[4] = { anchorXf + sb(28) * sxf, anchorXf + sb(29) * sxf,
                    anchorXf + sb(32) * sxf, anchorXf + sb(33) * sxf };
    float py[4] = { anchorYf + sb(30) * syf, anchorYf + sb(31) * syf,
                    anchorYf + sb(34) * syf, anchorYf + sb(35) * syf };
    int us[4] = { (int)(uv0 & 0xFFu), (int)(uv1 & 0xFFu), (int)(uv2 & 0xFFu), (int)(uv3 & 0xFFu) };
    int vs[4] = { (int)((uv0 >> 8) & 0xFFu), (int)((uv1 >> 8) & 0xFFu),
                  (int)((uv2 >> 8) & 0xFFu), (int)((uv3 >> 8) & 0xFFu) };

    unsigned char rr[4], gg[4], bb[4];
    for (int k = 0; k < 4; k++) {
      const uint32_t col = c->mem_r32(rec + 12u + (uint32_t)k * 4u);   // IR0 = 0 -> cue is the identity
      const unsigned char cr = (unsigned char)(col & 0xFFu);
      const unsigned char cg = (unsigned char)((col >> 8) & 0xFFu);
      const unsigned char cb = (unsigned char)((col >> 16) & 0xFFu);
      const int32_t fr = farColour ? farColour[0] : 0;
      const int32_t fg = farColour ? farColour[1] : 0;
      const int32_t fb = farColour ? farColour[2] : 0;
      rr[k] = ir0 ? SpriteAnchor::depthCue(cr, ir0, fr) : cr;
      gg[k] = ir0 ? SpriteAnchor::depthCue(cg, ir0, fg) : cg;
      bb[k] = ir0 ? SpriteAnchor::depthCue(cb, ir0, fb) : cb;
    }
    const uint16_t clut  = (uint16_t)(uv0 >> 16);
    const uint16_t tpage = (uint16_t)((uv1 >> 16) & 0x7Fu);
    c->game->activeRq().drawWorldQuad(c, px, py, depth, us, vs, rr, gg, bb, tpage, clut,
                                      /*semi=*/1, nullptr);
    drawn++;
    if ((int32_t)uv1 <= 0) break;                    // the terminal record IS drawn, then stop
  }
  return drawn;
}

}  // namespace

// The node's own render fn IS the emitter for every plain member of the family. It stops being so for
// a COMPOSITE dispatcher (see impactBurstRender below), which is why the emitter is a parameter of
// fxSpriteEmit rather than something it re-reads from the node.
void Render::fxSpriteRender(uint32_t node) {
  fxSpriteEmit(node, mCore->mem_r32(node + kRenderFn));
}

// FUN_80033080 — the WEAPON-IMPACT burst (kanban #15), a COMPOSITE render fn: it is nothing but
//     { FUN_80027E5C(node); FUN_800288AC(node); }
// i.e. one node drawn by TWO different effect families at once — this file's byte-scaled sprite
// (the white starburst flash) and fx_mesh.cpp's effect-mesh controller (the expanding radial plume).
//
// The mesh half already reaches the picture: 0x800288AC is scoped by fx_mesh.cpp's armTap, so the
// shared writer's quads are captured at guest-execution time. Measured on
// replays/bugs/weapon-impact-bucket.pad: 28 live quads over 9 animation steps, growing from a point
// to ~70px. So "the impact effect is missing" was never a whole-effect gap — it is the SPRITE half
// that had no producer, because pc_render's type-0x20 whitelist keys on the NODE's render fn and
// 0x80033080 is not itself an emitter address, so neither half of the pair was recognised.
//
// The sprite half is the FN_BYTESCALE variant (scale = MAC0 * node[6] >> 4). Passing that emitter
// explicitly is the whole fix — everything else in the family's contract is unchanged.
void Render::impactBurstRender(uint32_t node) {
  fxSpriteEmit(node, FN_BYTESCALE);
}

void Render::fxSpriteEmit(uint32_t node, uint32_t rfn) {
  Core* c = mCore;
  const uint32_t rec0 = c->mem_r32(node + kRecList);
  if (!rec0) return;                                        // no record list -> the emitter emits nothing
  const uint32_t clutPage = c->mem_r32(node + kClutPage);
  const int bias          = (int16_t)c->mem_r16(node + kOtBias);

  // The scene camera, fps60-lerped at the interp present (the whole reason this producer interpolates).
  EObjXform cam; projComposeCamera(&cam);
  const uint32_t H = (uint32_t)cam.H;

  ObjScope objScope(c, node);   // prim identity (dbg_node) = the emitting flame object

  // numerY defaults to numer — every emitter but FN_XYSCALE scales both axes by the same modulator.
  auto projectEmit = [&](int vx, int vy, int vz, int dqa, int32_t numer, int shift,
                         int32_t numerY, bool haveY = false) {
    ProjVtx pv; cam.project(vx, vy, vz, &pv);
    if (!SpriteAnchor::otKeyInRange(pv.sz, bias)) return;   // same emit/skip gate the emitter applies
    const int32_t base = SpriteAnchor::baseScale(H, pv.sz, dqa);
    int32_t sx = base, sy = base;
    if (shift) {                                            // E5C / per-particle / XY modulation
      sx = (int32_t)(((int64_t)base * numer) >> shift);
      sy = (int32_t)(((int64_t)base * (haveY ? numerY : numer)) >> shift);
    }
    spriteRecordsEmit(rec0, clutPage, pv.px, pv.py, proj_pz_to_ord(pv.pz), sx, sy);
    // `PSXPORT_DEBUG=fxsprite` — the family's counterpart to fxmesh/fxanim. It answers the one
    // question a whitelist change raises: did this producer fire for this node, and with what
    // geometry? A silently-skipped emitter (record list empty, anchor behind the camera, OT key out
    // of range) and a producer that was never dispatched look identical in the picture.
    if (cfg_dbg("fxsprite"))
      cfg_logf("fxsprite", "node=%08X emitter=%08X anchor=(%d,%d,%d) -> (%.1f,%.1f) sz=%d scale=%d,%d",
               node, rfn, vx, vy, vz, (double)pv.px, (double)pv.py, pv.sz, sx, sy);
  };

  if (rfn == FN_PARTICLE) {
    const int dqa   = (c->mem_r8(node + kVariant3) == '!') ? 4 : 6;
    const int count = (int16_t)c->mem_r16(node + kPartCount);
    for (int i = 0; i < count; i++) {
      const uint32_t p   = node + kPartArray + (uint32_t)i * 8u;
      const uint32_t axy = c->mem_r32(p);
      const uint32_t az  = c->mem_r32(p + 4u);
      const int16_t  mod = (int16_t)c->mem_r16(p + kPartScale);
      projectEmit((int16_t)axy, (int16_t)(axy >> 16), (int16_t)az, dqa, mod, 8, 0);
    }
    return;
  }

  if (rfn == FN_RINGROT) {
    // The node's own rotated frame, built host-side from the same LUT Math::rotmat reads.
    int32_t M[3][3];
    MeshQuads::rotmat(c, (int16_t)c->mem_r16(node + kRingAngles),
                         (int16_t)c->mem_r16(node + kRingAngles + 2),
                         (int16_t)c->mem_r16(node + kRingAngles + 4), M);
    float Robj[3][3];
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) Robj[i][j] = (float)M[i][j] / 4096.0f;
    const uint32_t pxy = c->mem_r32(node + kAnchorXY);
    const float Tobj[3] = { (float)(int16_t)pxy, (float)(int16_t)(pxy >> 16),
                            (float)(int16_t)c->mem_r32(node + kAnchorZ) };
    EObjXform ring; projComposeObjectHost(Robj, Tobj, &ring);
    const int height = (int)(int16_t)c->mem_r16(node + kRingHeight) << 6;
    const uint32_t ringH = (uint32_t)ring.H;
    for (int i = 0; i < kRingPoints; i++) {
      int sn, cs; MeshQuads::trig(c, i << 10, &sn, &cs);       // 0/1024/2048/3072 = the 4 cardinals
      ProjVtx pv;
      ring.project((cs * kRingRadiusN) >> 4, height, (sn * kRingRadiusN) >> 4, &pv);
      if (!SpriteAnchor::otKeyInRange(pv.sz, bias)) continue;  // the emitter's own per-point gate
      const int32_t scale = SpriteAnchor::baseScale(ringH, pv.sz, /*dqa=*/6);
      spriteRecordsEmit(rec0, clutPage, pv.px, pv.py, proj_pz_to_ord(pv.pz), scale, scale);
    }
    return;
  }

  // FN_UNIFORM / FN_BYTESCALE / FN_XYSCALE: one world anchor.
  const uint32_t axy = c->mem_r32(node + kAnchorXY);
  const uint32_t az  = c->mem_r32(node + kAnchorZ);
  if (rfn == FN_XYSCALE) {
    const int16_t mx = (int16_t)c->mem_r16(node + kXScaleMul);
    const int16_t my = (int16_t)c->mem_r16(node + kYScaleMul);
    projectEmit((int16_t)axy, (int16_t)(axy >> 16), (int16_t)az, /*dqa=*/6, mx, 8, my, /*haveY=*/true);
    return;
  }
  const int numer = (rfn == FN_BYTESCALE) ? (int)(uint8_t)c->mem_r8(node + kE5cScale) : 0;
  const int shift = (rfn == FN_BYTESCALE) ? 4 : 0;
  projectEmit((int16_t)axy, (int16_t)(axy >> 16), (int16_t)az, /*dqa=*/6, numer, shift, 0);
}

// The FUN_800286CC emitter, rebuilt: read the effect node's own animation cursor + world anchor, project
// with the native (fps60-lerped) camera and emit this frame's quad list. Nothing here runs a gen body and
// nothing writes guest memory — the guest still executes its own emitter underneath for state fidelity,
// this producer simply owns the picture.
void Render::fxAnimSpriteRender(uint32_t node) {
  Core* c = mCore;
  const uint32_t script = c->mem_r32(node + kAnimScript);
  const uint32_t table  = c->mem_r32(node + kAnimTable);
  if (!script || !table) return;                              // no animation -> the emitter emits nothing
  const uint32_t animFrame = (uint32_t)c->mem_r8(script) & 0x7Fu;    // bit 7 = the guest's loop marker
  const uint32_t rec0 = c->mem_r32(table + animFrame * 4u);
  if (!rec0) return;

  // The scene camera, fps60-lerped at the interp present — the reason the puff interpolates.
  EObjXform cam; projComposeCamera(&cam);
  const uint32_t H = (uint32_t)cam.H;
  const int bias = (int16_t)c->mem_r16(node + kOtBias);
  const uint32_t axy = c->mem_r32(node + kAnchorXY);
  const uint32_t az  = c->mem_r32(node + kAnchorZ);

  // The anchor through the actor-transform interpolation tier: on the fps60 in-between present it comes
  // back blended with last frame's, so a puff that is travelling moves between the two real frames.
  EffectPoints live;
  live.n = 1; live.valid[0] = true;
  live.x[0] = (int16_t)axy; live.y[0] = (int16_t)(axy >> 16); live.z[0] = (int16_t)az;
  const EffectPoints& pts = mEffectLerp.resolve(c, node, live);

  ProjVtx pv; cam.project(pts.x[0], pts.y[0], pts.z[0], &pv);
  if (!SpriteAnchor::otKeyInRange(pv.sz, bias)) return;       // the emitter's own emit/skip decision

  const int32_t mac0 = SpriteAnchor::baseScale(H, pv.sz, kAnimDqa);
  const uint32_t sc  = c->mem_r32(node + kAnimScale);
  const int32_t scaleX = (int32_t)(((int64_t)mac0 * (int64_t)(uint16_t)sc) >> 8);
  const int32_t scaleY = (int32_t)(((int64_t)mac0 * (int64_t)(uint16_t)(sc >> 16)) >> 8);

  // DEPTH: this family authors a NEAR BIAS at node+0x32 and means it — the impact starburst carries -50
  // and is drawn over the character it bursts on. The bias is stated in OT buckets, and the bucket key is
  // SZ3>>2, so in view-Z units it is 4·bias; applying it here is the effect's own authored depth, not a
  // fudge. (Without it a flat anchor-depth sprite loses its near half to whatever it is bursting on.)
  float pz = pv.pz + (float)(4 * bias);
  const float nearPz = proj_near_pz();
  if (pz < nearPz) pz = nearPz;

  ObjScope objScope(c, node);   // prim identity (dbg_node) = the emitting effect object
  const int quads = emitAnimQuadRecords(c, rec0, pv.px, pv.py, proj_pz_to_ord(pz), scaleX, scaleY);
  if (cfg_dbg("fxanim"))
    cfg_logf("fxanim", "node=%08X frame=%u rec0=%08X anchor=(%.1f,%.1f) sz=%d scale=(%d,%d) quads=%d",
             node, animFrame, rec0, (double)pv.px, (double)pv.py, pv.sz, scaleX, scaleY, quads);
}

// ── FUN_800328EC family producers ────────────────────────────────────────────────────────────────
// Shared tail: project the node's own world anchor with the native (fps60-lerped) camera, apply the
// emitter's own OT-key gate, and hand the model list to the SAME four-corner writer producer
// fxAnimSpriteRender uses. Nothing runs a gen body; nothing writes guest memory — in particular the
// guest's own animation-cursor store at node+0x68 stays the guest's, since its body still executes
// underneath. Read-only.
void Render::altSpriteEmit(const AltSprite& a) {
  Core* c = mCore;
  if (!a.rec0) return;

  EObjXform cam; projComposeCamera(&cam);
  ProjVtx pv;
  cam.project((int16_t)c->mem_r16(a.node + a.anchorX), (int16_t)c->mem_r16(a.node + a.anchorX + 4),
              (int16_t)c->mem_r16(a.node + a.anchorX + 8), &pv);
  if (!SpriteAnchor::otKeyInRange(pv.sz, a.gateBias)) return;   // FUN_800317CC's emit/skip decision

  const int32_t mac0 = SpriteAnchor::baseScale((uint32_t)cam.H, pv.sz, a.dqa);
  const int32_t sx = (int32_t)(((int64_t)mac0 * (int64_t)a.numerX) >> a.shift);
  const int32_t sy = (int32_t)(((int64_t)mac0 * (int64_t)a.numerY) >> a.shift);

  // The authored near bias, in the same units and for the same reason as fxAnimSpriteRender: the gate
  // states it in OT buckets and the bucket key is SZ3>>2, so one unit is 4 units of view depth.
  // It is a SEPARATE number from the gate's bias because one controller (FUN_8012D9E8) gates with 0
  // and only then subtracts 100 from the resulting key — gating and depth-biasing are two decisions.
  float pz = pv.pz + (float)(4 * a.depthBias);
  const float nearPz = proj_near_pz();
  if (pz < nearPz) pz = nearPz;

  ObjScope objScope(c, a.node);
  const int quads = emitAnimQuadRecords(c, a.rec0, pv.px, pv.py, proj_pz_to_ord(pz), sx, sy);
  if (cfg_dbg("fxsprite"))
    cfg_logf("fxsprite", "alt node=%08X rec0=%08X dqa=%d gate=%d depth=%d anchor=(%.1f,%.1f) sz=%d "
                         "scale=(%d,%d) quads=%d",
             a.node, a.rec0, a.dqa, a.gateBias, a.depthBias,
             (double)pv.px, (double)pv.py, pv.sz, sx, sy, quads);
}

// FUN_8012E868 (A01 overlay) — the animation-script member of the family. Same shape as
// fxAnimSpriteRender, different offsets: anchor as three separate s16s, scale pair at node+0x60,
// script pointer at node+0x64, per-frame record table at node+0x6C, DQA 6 and no OT bias.
void Render::fxAltAnimSpriteRender(uint32_t node) {
  Core* c = mCore;
  const uint32_t script = c->mem_r32(node + kAltScript);
  const uint32_t table  = c->mem_r32(node + kAltTable);
  if (!script || !table) return;                      // the guest's own `if (ptr == 0) return`
  const uint32_t frame = (uint32_t)c->mem_r8(script) & 0x7Fu;   // bit 7 = the loop marker
  const uint32_t sc = c->mem_r32(node + kAltScale);
  AltSprite a;
  a.node = node; a.anchorX = kAltAnchorX; a.dqa = 6;
  a.rec0 = c->mem_r32(table + frame * 4u);
  a.numerX = (uint16_t)sc; a.numerY = (uint16_t)(sc >> 16);
  altSpriteEmit(a);
}

// FUN_8013D454's SPRITE branch — the water jet's other half. fx_mesh.cpp's scope owns the mesh branch
// (non-zero node+0x60, drawn through FUN_80027768); this owns the zero branch, which emits through
// FUN_800328EC and therefore had no producer at all even after the scope wrappers landed. The two
// branches are mutually exclusive, so there is no double-draw.
void Render::waterJetSpriteRender(uint32_t node) {
  Core* c = mCore;
  if ((int16_t)c->mem_r16(node + kJetMode) != 0) return;        // the mesh branch: not ours
  const uint32_t numer = (uint16_t)c->mem_r16(node + kJetScale);
  AltSprite a;
  a.node = node; a.anchorX = kAltAnchorX; a.dqa = kJetDqa;
  a.gateBias = kJetBias; a.depthBias = kJetBias;
  a.rec0 = c->mem_r32(kJetModelTab);
  a.numerX = numer; a.numerY = numer;
  altSpriteEmit(a);
}

// FUN_8012D9E8's SPRITE TAIL. This controller is two emitters in one function: a large inline rotated
// MESH pass (rotmat from node+0x54, column-scaled by the triple at node+0x68/0x6A/0x6C, composed with
// the scene camera, then a 36-byte-record loop that RTPTs each quad and links its packet by hand —
// still unported, and the reason this entry only claims the tail), followed by this sprite.
//
// The tail differs from its siblings in three measurable ways, which is why AltSprite carries them as
// fields rather than as constants: the anchor is the packed pair at node+0x60/0x64 (not the three
// separate s16s at 0x2E), the OT key is gated with bias 0 and only THEN has 100 subtracted from it
// (so the gate and the depth bias are genuinely different numbers), and the scale shift is 11, not 8.
// The model comes from one of two tables chosen by node+3, indexed by the signed high byte of
// node+0x40.
void Render::fxRotSpriteTailRender(uint32_t node) {
  Core* c = mCore;
  const uint32_t table = (c->mem_r8(node + 3) == kRotTailAltSel) ? kRotTailTableA : kRotTailTableB;
  const int idx = (int)(int16_t)c->mem_r16(node + kRotTailIndex) >> 8;   // the signed high byte
  if (idx < 0 || idx >= kRotTailTableN) return;
  AltSprite a;
  a.node = node; a.anchorX = kRotTailAnchor; a.dqa = 6;
  a.gateBias = 0; a.depthBias = kRotTailDepthBias;
  a.shift = kRotTailShift;
  a.rec0 = c->mem_r32(table + (uint32_t)idx * 4u);
  a.numerX = a.numerY = c->mem_r16s(node + kRotTailScale);   // SIGNED: gen uses lh, not lhu
  altSpriteEmit(a);
}

// FUN_80113768 (A0A overlay, area 10) — surfaced by the 22-AREA nofx sweep, which no replay in the
// library reaches. A fifth member of the FUN_80027A4C family, and the first one that actually DRIVES
// the writer's depth cue instead of programming the identity.
//
// RE from ov_a0a_gen_80113768. Standard family opening — FUN_800329E0(6) for the scene camera and
// DQA, the packed anchor at node+0x2C/+0x30, FUN_800317CC gated on the node's own (s16)node+0x32 —
// then three things worth naming:
//   * the writer is FUN_800328BC, which is itself a wrapper: FUN_80027A4C(recList, node+0x44 word).
//     So spriteRecordsEmit already owns the record walk; only the dispatch was missing.
//   * the scale is MAC0 >> 8 FIRST, then multiplied by (s16)node+0x48 / node+0x4A — the reverse order
//     from FN_XSCALE's multiply-then-shift. Same algebra, different rounding, so it is reproduced in
//     the order the guest performs it rather than folded.
//   * IR0 = (u8)node[7] << 5 with the far colour zeroed, i.e. a REAL depth cue: this effect fades its
//     record colours toward black under an animator, which is exactly what the ir0 parameter of
//     spriteRecordsEmit exists for and what every other member leaves at the identity.
// The gate's FAIL path calls FUN_80031780, which only advances the node's own record cursor at
// node+0x38 — guest state, no drawing — so a skipped emit correctly produces no picture here.
void Render::fxCuedSpriteRender(uint32_t node) {
  Core* c = mCore;
  const uint32_t rec0 = c->mem_r32(node + kCuedRecList);
  if (!rec0) return;                                  // the guest's own `if (list == 0) return`

  EObjXform cam; projComposeCamera(&cam);
  const int bias = (int16_t)c->mem_r16(node + kOtBias);
  const uint32_t axy = c->mem_r32(node + kAnchorXY), az = c->mem_r32(node + kAnchorZ);
  ProjVtx pv;
  cam.project((int16_t)axy, (int16_t)(axy >> 16), (int16_t)az, &pv);
  if (!SpriteAnchor::otKeyInRange(pv.sz, bias)) return;

  const int32_t base = SpriteAnchor::baseScale((uint32_t)cam.H, pv.sz, 6) >> 8;   // shift, THEN scale
  const int32_t sx = base * (int32_t)(int16_t)c->mem_r16(node + kCuedScaleX);
  const int32_t sy = base * (int32_t)(int16_t)c->mem_r16(node + kCuedScaleY);
  const int32_t ir0 = (int32_t)(uint32_t)c->mem_r8(node + kCuedCueByte) << kCuedCueShift;
  const int32_t farBlack[3] = { 0, 0, 0 };            // the guest zeroes CR21-23 before the writer

  ObjScope objScope(c, node);
  spriteRecordsEmit(rec0, c->mem_r32(node + kClutPage), pv.px, pv.py, proj_pz_to_ord(pv.pz),
                    sx, sy, ir0, farBlack);
  if (cfg_dbg("fxsprite"))
    cfg_logf("fxsprite", "cued node=%08X rec0=%08X anchor=(%.1f,%.1f) sz=%d scale=(%d,%d) ir0=%d",
             node, rec0, (double)pv.px, (double)pv.py, pv.sz, sx, sy, ir0);
}

// ── FUN_80110C14 (A0D overlay, area 13) — the ORBITING SPRITE RING ────────────────────────────────
// The sixth FUN_80027A4C-family member, and the first that draws MANY sprites from ONE node: 21 items
// ride an arc around the node's own world anchor, each with its own radius, vertical bob and size.
// Surfaced by the 22-area nofx sweep (no replay in the library reaches it) — same provenance as
// fxCuedSpriteRender above.
//
// RE from ov_a0d_gen_80110C14 (generated/ov_a0d_shard_1.c:3370). The opening is the family standard —
// FUN_800329E0(6) for the scene camera and DQA, then *(u32*)0x1F800090 = 0 so IR0 is the identity and
// every item's record colours pass through unchanged. What is new is the per-item table:
//
//   node+0x34 holds 21 four-byte items { spread, bob, phase, size }, and for item i:
//     angle  = 1024 + 97*i                                  PSX angle units, 4096 = one turn
//     radius = (phase * spread) >> 5 + (s16)node+0x32       node+0x32 is a base RADIUS here, not an
//                                                           OT bias — this controller's own layout
//     VX = node+0x2C + (rsin(angle) * radius >> 12)
//     VZ = node+0x30 + (rcos(angle) * radius >> 12)
//     VY = node+0x2E - (rsin(phase << 4) * bob >> 12)       each item bobs on its OWN phase, which is
//                                                           why the ring never looks rigid
//     FUN_800317CC(-50) gates the OT key, then the MAC0 it publishes is scaled by `size` >> 7 and
//     written to BOTH axis slots (0x1F800084/88), so items are uniformly scaled.
//     The record list is picked by the phase byte's HIGH NIBBLE out of a 16-slot bank in MAIN.EXE:
//     0x8009D5FC + (phase & 0xF0), 16 bytes per slot = two 8-byte records, terminal bit on the second.
//
// The guest builds each anchor as three 16-bit stores and lets them wrap, so the arithmetic is done in
// uint16_t here and read back signed — the same treatment fx_vortex.cpp's particle sphere gives its
// ring points. Read-only: nothing here writes guest memory.
constexpr uint32_t kRingTable     = 0x34u;        // 21 x { u8 spread, u8 bob, u8 phase, u8 size }
constexpr uint32_t kRingItemSize  = 4u;
constexpr int      kRingItems     = 21;
constexpr uint32_t kRingAnchorVX  = 0x2Cu;        // three consecutive u16: VX, VY at +2, VZ at +4
constexpr uint32_t kRingBaseRadius = 0x32u;       // s16 radius added to every item's spread
constexpr int      kRingAngle0    = 1024;
constexpr int      kRingAngleStep = 97;
constexpr int      kRingGateBias  = -50;          // the emitter's authored near bias, in OT buckets
constexpr int      kRingDqa       = 6;
constexpr uint32_t kRingRecBank   = 0x8009D5FCu;  // MAIN.EXE-resident record bank, 16-byte slots
constexpr uint32_t kRingClutPtr   = 0x8009D5F8u;  // -> the bank's shared clut (lo16) | tpage (hi16)

void Render::fxRingSpriteRender(uint32_t node) {
  Core* c = mCore;

  EObjXform cam; projComposeCamera(&cam);
  const uint32_t H = (uint32_t)cam.H;
  const uint32_t clutPage = c->mem_r32(kRingClutPtr);
  const int baseRadius = (int16_t)c->mem_r16(node + kRingBaseRadius);
  const uint16_t anchorX = c->mem_r16(node + kRingAnchorVX);
  const uint16_t anchorY = c->mem_r16(node + kRingAnchorVX + 2u);
  const uint16_t anchorZ = c->mem_r16(node + kRingAnchorVX + 4u);
  const Trig& trig = trigOf(c);

  // The authored near bias, in the units the rest of this file uses: the gate states it in OT buckets
  // and the bucket key is SZ3>>2, so one unit is four of view depth. -50 is the same value the impact
  // starburst carries, and it means the same thing — draw the ring in front of what it surrounds.
  const float nearPz = proj_near_pz();

  ObjScope objScope(c, node);   // prim identity for the whole ring = the emitting effect object
  int drawn = 0;
  float minPx = 0, maxPx = 0, minPy = 0, maxPy = 0;   // screen extent of what was actually emitted
  for (int i = 0; i < kRingItems; i++) {
    const uint32_t item = node + kRingTable + (uint32_t)i * kRingItemSize;
    const uint32_t spread = c->mem_r8(item + 0u);
    const uint32_t bob    = c->mem_r8(item + 1u);
    const uint32_t phase  = c->mem_r8(item + 2u);
    const uint32_t size   = c->mem_r8(item + 3u);

    const int angle  = (int16_t)(kRingAngle0 + kRingAngleStep * i);
    const int radius = (int)(((int32_t)(phase * spread)) >> 5) + baseRadius;

    const int32_t offX = (int32_t)(((int64_t)trig.rsin(angle) * radius) >> 12);
    const int32_t offZ = (int32_t)(((int64_t)trig.rcos(angle) * radius) >> 12);
    const int32_t offY = (int32_t)(((int64_t)trig.rsin((int)(phase << 4)) * (int32_t)bob) >> 12);

    const int16_t vx = (int16_t)(uint16_t)(anchorX + (uint16_t)offX);
    const int16_t vy = (int16_t)(uint16_t)(anchorY - (uint16_t)offY);
    const int16_t vz = (int16_t)(uint16_t)(anchorZ + (uint16_t)offZ);

    ProjVtx pv;
    cam.project(vx, vy, vz, &pv);
    if (!SpriteAnchor::otKeyInRange(pv.sz, kRingGateBias)) continue;   // the emitter's own skip

    const int32_t scale = (int32_t)(((int64_t)SpriteAnchor::baseScale(H, pv.sz, kRingDqa) * size) >> 7);
    const uint32_t recList = kRingRecBank + (phase & 0xF0u);

    float pz = pv.pz + (float)(4 * kRingGateBias);
    if (pz < nearPz) pz = nearPz;
    spriteRecordsEmit(recList, clutPage, pv.px, pv.py, proj_pz_to_ord(pz), scale, scale, 0, nullptr);
    if (!drawn) { minPx = maxPx = pv.px; minPy = maxPy = pv.py; }
    else {
      if (pv.px < minPx) minPx = pv.px;  if (pv.px > maxPx) maxPx = pv.px;
      if (pv.py < minPy) minPy = pv.py;  if (pv.py > maxPy) maxPy = pv.py;
    }
    drawn++;
  }

  // Log the SCREEN extent, the way the rest of the family logs its anchor — a ring that passes the OT
  // gate can still land wholly off-frame, and world coordinates cannot tell you which happened.
  if (cfg_dbg("fxsprite"))
    cfg_logf("fxsprite", "ring node=%08X world=(%d,%d,%d) baseR=%d clut=%08X drawn=%d/%d "
                         "screen x[%.1f..%.1f] y[%.1f..%.1f]",
             node, (int)(int16_t)anchorX, (int)(int16_t)anchorY, (int)(int16_t)anchorZ,
             baseRadius, clutPage, drawn, kRingItems,
             (double)minPx, (double)maxPx, (double)minPy, (double)maxPy);
}

// ── FUN_8010C7F4 (A0L overlay, area 21) — the PARTICLE FIELD ──────────────────────────────────────
// The area Tomba rides a bird through: 64 particles scattered around a moving base point, drifting on
// a wind angle and fading to black with distance. Also from the 22-area nofx sweep.
//
// RE from ov_a0l_gen_8010C7F4 (generated/ov_a0l_shard_0.c:1479). It belongs to the FOUR-CORNER writer
// family (FUN_8002847C = emitAnimQuadRecords) rather than the 8-byte one, and it is the only member so
// far that DRIVES the depth cue:
//   * four record lists are copied at entry from 0x80109068 and cycled per particle by i & 3.
//   * GTE CR21-23 (the far colour) are zeroed, so the cue fades toward BLACK.
//   * a wind drift comes from the angle at 0x800E7ED6: rcos/rsin of it, DOUBLED, times ten times the
//     magnitude at node+0x50, >> 12. It is added to the random X and Z before the mask, so the whole
//     field leans with the wind instead of each particle leaning independently.
//   * positions are an LCG (seed 0x12D687, multiplier at 0x80115894, x = x*mult + 1) stepped THREE
//     times per particle — X takes the value before the first step, Y after the first, Z after the
//     second — masked to 14 bits and centred: base + (rand & 0x3FFF) - 8192, base being the three
//     s16 at 0x1F800160/62/64.
//   * IR0 = Trig::vecLen(particle - (s16)0x1F8000D2/D6/DA) >> 3, i.e. the depth cue is the DISTANCE
//     from a reference point, which is why the field thins out with range.
//   * gate FUN_800317CC(0), then the published MAC0 is DOUBLED into both axis slots.
// Read-only: the LCG is a local register in the guest too, so nothing here writes guest memory.
constexpr uint32_t kFieldRecTable  = 0x80109068u;   // four record-list pointers, cycled by i & 3
constexpr uint32_t kFieldWindAngle = 0x800E7ED6u;   // s16 wind angle
constexpr uint32_t kFieldMagnitude = 0x50u;         // node+0x50, the wind magnitude
constexpr uint32_t kFieldBaseXYZ   = 0x1F800160u;   // three s16: the field's centre
constexpr uint32_t kFieldDistRef   = 0x1F8000D2u;   // three s16 at +0/+4/+8: the depth-cue origin
constexpr uint32_t kFieldLcgMult   = 0x80115894u;
constexpr uint32_t kFieldLcgSeed   = 0x0012D687u;
constexpr int      kFieldCount     = 64;
constexpr int      kFieldDqa       = 6;
constexpr uint32_t kFieldSpread    = 0x3FFFu;       // 14-bit mask ...
constexpr int32_t  kFieldCentre    = 8192;          // ... centred, so offsets run [-8192, 8191]

void Render::fxParticleFieldRender(uint32_t node) {
  Core* c = mCore;

  EObjXform cam; projComposeCamera(&cam);
  const uint32_t H = (uint32_t)cam.H;
  const Trig& trig = trigOf(c);

  // The wind drift: doubled cos/sin of the scene's wind angle against ten times the node's magnitude.
  const int windAngle = (int16_t)c->mem_r16(kFieldWindAngle);
  const int32_t mag10 = (int32_t)c->mem_r32(node + kFieldMagnitude) * 10;
  const int32_t windX = (int32_t)(((int64_t)(trig.rcos(windAngle) * 2) * mag10) >> 12);
  const int32_t windZ = (int32_t)(((int64_t)(trig.rsin(windAngle) * 2) * mag10) >> 12);

  const int32_t baseX = (int16_t)c->mem_r16(kFieldBaseXYZ);
  const int32_t baseY = (int16_t)c->mem_r16(kFieldBaseXYZ + 2u);
  const int32_t baseZ = (int16_t)c->mem_r16(kFieldBaseXYZ + 4u);
  const int32_t refX  = (int16_t)c->mem_r16(kFieldDistRef);
  const int32_t refY  = (int16_t)c->mem_r16(kFieldDistRef + 4u);
  const int32_t refZ  = (int16_t)c->mem_r16(kFieldDistRef + 8u);
  const int32_t farBlack[3] = { 0, 0, 0 };            // the guest zeroes CR21-23 before the writer

  uint32_t recList[4];
  for (int k = 0; k < 4; k++) recList[k] = c->mem_r32(kFieldRecTable + (uint32_t)k * 4u);

  const uint32_t lcgMult = c->mem_r32(kFieldLcgMult);
  uint32_t lcg = kFieldLcgSeed;

  ObjScope objScope(c, node);
  int drawn = 0, quads = 0;
  float minPx = 0, maxPx = 0, minPy = 0, maxPy = 0;
  for (int i = 0; i < kFieldCount; i++) {
    // THREE LCG steps per particle, and the axes read it at three different points in that chain —
    // reproduced in the guest's order because the sequence is what places the field.
    const int32_t rx = (int32_t)((lcg + (uint32_t)windX) & kFieldSpread);  lcg = lcg * lcgMult + 1u;
    const int32_t ry = (int32_t)(lcg & kFieldSpread);                      lcg = lcg * lcgMult + 1u;
    const int32_t rz = (int32_t)((lcg + (uint32_t)windZ) & kFieldSpread);  lcg = lcg * lcgMult + 1u;

    const int16_t vx = (int16_t)(uint16_t)(baseX + rx - kFieldCentre);
    const int16_t vy = (int16_t)(uint16_t)(baseY + ry - kFieldCentre);
    const int16_t vz = (int16_t)(uint16_t)(baseZ + rz - kFieldCentre);

    const int32_t ir0 = Trig::vecLen(vx - refX, vy - refY, vz - refZ) >> 3;

    ProjVtx pv;
    cam.project(vx, vy, vz, &pv);
    if (!SpriteAnchor::otKeyInRange(pv.sz, 0)) continue;   // the emitter's own emit/skip decision

    const int32_t scale = SpriteAnchor::baseScale(H, pv.sz, kFieldDqa) << 1;
    quads += emitAnimQuadRecords(c, recList[i & 3], pv.px, pv.py, proj_pz_to_ord(pv.pz),
                                 scale, scale, ir0, farBlack);
    if (!drawn) { minPx = maxPx = pv.px; minPy = maxPy = pv.py; }
    else {
      if (pv.px < minPx) minPx = pv.px;  if (pv.px > maxPx) maxPx = pv.px;
      if (pv.py < minPy) minPy = pv.py;  if (pv.py > maxPy) maxPy = pv.py;
    }
    drawn++;
  }

  if (cfg_dbg("fxsprite"))
    cfg_logf("fxsprite", "field node=%08X base=(%d,%d,%d) wind=(%d,%d)@%d drawn=%d/%d quads=%d "
                         "screen x[%.1f..%.1f] y[%.1f..%.1f]",
             node, baseX, baseY, baseZ, windX, windZ, windAngle, drawn, kFieldCount, quads,
             (double)minPx, (double)maxPx, (double)minPy, (double)maxPy);
}
