// game/render/fx_trail.cpp — the SCREEN-SPACE ADDITIVE MOTION TRAIL, guest FUN_801113B4 (A03
// overlay, area 3). Found by the 22-area nofx sweep; per claim C012 it currently draws nothing.
//
// THIS IS NOT A SPRITE-FAMILY MEMBER, and it must not be made one. It never touches the GTE, never
// projects anything, and never calls FUN_800329E0 / FUN_800317CC / FUN_80027A4C / FUN_8002847C — so
// SpriteAnchor::baseScale and SpriteAnchor::otKeyInRange DO NOT APPLY here (it programs no DQA, so a
// baseScale would be a fabricated number). Its geometry is already in screen pixels.
//
// RE from overlay guest 0x801113B4 + overlay guest 0x80110B00, statically verified 2026-07-28 (spec and the
// adversarial verifier's corrections: docs/re/render-targets-binary-analysis.md).
//
// WHAT IT IS: an 11-slot SCREEN-POSITION HISTORY at node+0x3C — {s16 x, s16 y} per slot, (0,0) means
// "not filled yet" — joined into a continuous glowing ribbon. Each of the 10 joints is drawn as a
// bright ~2px core (the ramp colour along the centre line, fading to BLACK at the outer edge) inside
// an 8px halo of the same gradient at half brightness: the textbook two-tier glow. An 11-entry colour
// ramp at 0x80108FDC fades the ribbon from head to tail. The guest arms GP0 semi-transparency mode 1
// (100%B + 100%F, ADDITIVE) with its own DR_TPAGE and links everything into OT bucket 4, i.e. in
// front of essentially the whole 3D scene.
//
// THE WRAPPER'S ARGUMENTS ARE DEAD. FUN_801113B4 is eleven instructions: FUN_80110B00(4, node+0x3C,
// 16). In this overlay's body a0 is overwritten in the prologue and a2 is written before it is ever
// read; only node+0x3C survives. The values 4 and 16 match two hardcoded constants in the body (4
// quads per segment, a 16-word table), so they are vestigial parameters of a shared signature this
// build const-folded — do not plumb them through.
//
// READ-ONLY: the guest's writes are all packet-pool / OT / pool-cursor state, so a native producer
// reproduces the picture with zero guest writes. The guest's pool-room gate (a headroom check on
// 0x800BF544 against a parity-selected limit) is therefore NOT reproduced: it exists to stop the
// guest allocating, and this producer allocates nothing.
#include "cfg.h"
#include "core.h"
#include "fx_node.h" // FxNode — the walk-owned header lens this controller extends
#include "game.h"
#include "game_ctx.h" // trigOf(c)
#include "render.h"
#include "render_internal.h" // ObjScope
#include "render_queue.h"
#include "trig.h" // Trig::ratan2 / rcos / rsin — direct calls, see the note below
#include <cstdint>

// PROVENANCE NOTE, because the RE spec got this wrong and the verifier caught it: rsin/rcos/ratan2
// are NOT installed overrides. Trig::registerOverrides is deliberately an empty body — their
// substrate bodies descend a guest stack frame the native methods do not mirror, so registering them
// diverges SBS (game/math/trig.cpp's banner has the detail). Calling the Trig METHODS DIRECTLY, as
// this producer does, is the sanctioned use and is what that banner preserves them for.

namespace {

constexpr int kTrailSlots = 11;

// The A03 trail controller's OWN view of its node. The only field it owns is the history ring at
// +0x3C — SCREEN points, not world ones, which is why nothing here goes near the family's anchor
// slots. Naming it in a lens keeps that distinction at the call site.
class TrailNode : public FxNode {
public:
  using FxNode::FxNode;
  static constexpr uint32_t kPoints = 0x3Cu; // 11 x { s16 x, s16 y }, stride 4
  int32_t pointX(int i) const {
    return s16(kPoints + (uint32_t)i * 4u);
  }
  int32_t pointY(int i) const {
    return s16(kPoints + (uint32_t)i * 4u + 2u);
  }
};
constexpr uint32_t kTrailRamp = 0x80108FDCu; // 16 u32 colour words; only [0..10] are ever read
constexpr int kTrailQuarter = 1024;          // a quarter turn in PSX angle units (4096 = full)
constexpr int kTrailWide = 2;                // halo is 4x the core: perp << 2
constexpr int kTrailBlend = 3;               // RenderQueue semi blend selector
constexpr int kTrailMode = 3;                // untextured gouraud

// half(w): the guest's per-byte halving — a LOGICAL 32-bit shift then a per-byte 0x7F mask, so each
// RGB byte is halved with truncation and no carry leaks between channels.
inline uint32_t halfColour(uint32_t w) {
  return (w >> 1) & 0xFF7F7F7Fu;
}

struct Pt {
  int x, y;
};

} // namespace

// One joint = four quads. In all of them the two "far" vertices are BLACK (the outer edge) and the
// two centre-line vertices carry the ramp colour, which is what makes the ribbon glow rather than
// read as a flat band. v0 uses the JOINT perpendicular and v1 the current one, so consecutive quads
// share an exact edge and the ribbon is continuous instead of a chain of blobs.
void Render::fxMotionTrailRender(uint32_t node) {
  Core *c = mCore;
  const Trig &trig = trigOf(c);
  RenderQueue &rq = c->game->activeRq();

  const TrailNode tn(c, node);
  Pt prev{tn.pointX(0), tn.pointY(0)};

  // The degeneracy history. bit0 = "this point is null or a duplicate", bit1 = the PREVIOUS
  // iteration's bit0 — so one bad point suppresses the segment ending at it AND the next one.
  int hist = (prev.x == 0 && prev.y == 0) ? 1 : 0;

  int prevPerpX = 0, prevPerpY = 0;
  int drawn = 0;

  ObjScope objScope(c, node);
  for (int i = 1; i < kTrailSlots; i++) {
    const Pt cur{tn.pointX(i), tn.pointY(i)};

    hist = (hist << 1) & 3;
    if (cur.x == 0 && cur.y == 0) {
      hist |= 1;
    }
    if (cur.x == prev.x && cur.y == prev.y) {
      hist |= 1;
    }

    // Computed EVERY iteration, BEFORE the skip test — a suppressed segment still advances the joint
    // state, which is why a gap in the history does not kink the ribbon on the far side of it.
    const int ang = trig.ratan2(cur.y - prev.y, cur.x - prev.x) + kTrailQuarter;
    const int perpX = (trig.rcos(ang) * kTrailWide + 2048) >> 12; // arithmetic shift; lands in [-2,2]
    const int perpY = (trig.rsin(ang) * kTrailWide + 2048) >> 12;

    if (hist == 0) {
      // The joint rule keys on the LOOP COUNTER, not on "first emitted segment": if segment 1 was
      // suppressed, segment 2 still joins to segment 1's computed perpendicular.
      const int jx = (i == 1) ? perpX : prevPerpX;
      const int jy = (i == 1) ? perpY : prevPerpY;
      const uint32_t colA = c->mem_r32(kTrailRamp + (uint32_t)(i - 1) * 4u);
      const uint32_t colB = c->mem_r32(kTrailRamp + (uint32_t)i * 4u);

      // core -1x, core +1x, halo -4x, halo +4x. The halo shift happens AFTER the >>12 rounding, so
      // it is exactly four times the rounded core rather than a separately rounded value.
      for (int k = 0; k < 4; k++) {
        const int wide = (k >= 2) ? 4 : 1;
        const int sign = (k & 1) ? 1 : -1;
        const uint32_t cA = (k >= 2) ? halfColour(colA) : colA;
        const uint32_t cB = (k >= 2) ? halfColour(colB) : colB;

        // 16-bit wrapping, exactly as the guest stores each component with a halfword write.
        const int x0 = (int16_t)(uint16_t)(prev.x + sign * jx * wide);
        const int y0 = (int16_t)(uint16_t)(prev.y + sign * jy * wide);
        const int x1 = (int16_t)(uint16_t)(cur.x + sign * perpX * wide);
        const int y1 = (int16_t)(uint16_t)(cur.y + sign * perpY * wide);

        const int xs[4] = {x0, x1, prev.x, cur.x};
        const int ys[4] = {y0, y1, prev.y, cur.y};
        const float xsf[4] = {(float)x0, (float)x1, (float)prev.x, (float)cur.x};
        const float ysf[4] = {(float)y0, (float)y1, (float)prev.y, (float)cur.y};
        const unsigned char rr[4] = {0, 0, (unsigned char)(cA & 0xFFu), (unsigned char)(cB & 0xFFu)};
        const unsigned char gg[4] = {0, 0, (unsigned char)((cA >> 8) & 0xFFu), (unsigned char)((cB >> 8) & 0xFFu)};
        const unsigned char bb[4] = {0, 0, (unsigned char)((cA >> 16) & 0xFFu), (unsigned char)((cB >> 16) & 0xFFu)};
        const int uv[4] = {0, 0, 0, 0};
        const float depth[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        // LAYER CHOICE (the one judgement call the RE spec flagged): the guest links these into OT
        // bucket 4, in front of essentially the whole 3D scene, and the geometry is already in screen
        // pixels — so this is a 2D foreground overlay, not a depth-sorted world prim. RQ_OVERLAY
        // rather than RQ_HUD because it is an effect, not HUD chrome.
        rq.emitOrQueue(c,
                       /*capture=*/1,
                       RQ_OVERLAY,
                       RQ_OM_2D_FG,
                       /*nv=*/4,
                       /*semi=*/1,
                       /*raw=*/0,
                       xs,
                       ys,
                       xsf,
                       ysf,
                       uv,
                       uv,
                       rr,
                       gg,
                       bb,
                       depth,
                       kTrailMode,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       1023,
                       511,
                       kTrailBlend);
      }
      drawn++;
    }

    prevPerpX = perpX;
    prevPerpY = perpY; // the tail runs for suppressed segments too
    prev = cur;
  }

  if (cfg_dbg("fxtrail")) {
    cfg_logf("fxtrail", "trail node=%08X head=(%d,%d) segments=%d/%d", node, prev.x, prev.y, drawn, kTrailSlots - 1);
  }
}
