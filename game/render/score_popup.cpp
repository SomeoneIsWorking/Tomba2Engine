// class ScorePopup — implementation. See score_popup.h for the RE, the measurement that identified
// the missing layer, and why paint order comes from the guest's OT bucket.
#include "score_popup.h"
#include "core.h"
#include "game.h"
#include "game_ctx.h"        // eng(c) / rend(c)
#include "engine.h"
#include "render.h"          // Render::emitUiFt4 / emitUiSprites + rsub.mode.psxRender() gate
#include "render_queue.h"    // RQ_OVERLAY
#include <lucent/log.h>      // `scorepopup` diagnostic channel
#include <algorithm>
#include <numeric>

extern void gen_func_80072520(Core*);
extern void engine_set_override_main(uint32_t, OverrideFn, OverrideFn);

void ScorePopup::collect(Core* c, const UiGroupArgs& a) {
  if (c->game->oracle || c->rsub.mode.psxRender()) return;   // read-only overlay gate
  ScorePopup& popup = eng(c).scorePopup;
  if (!popup.mInDraw) return;                 // every other caller owns its own producer
  popup.mGroups.push_back(a);
}

void ScorePopup::drawCollected() {
  Core* c = core;
  if (mGroups.empty()) return;

  // Paint order = the guest's ordering table: buckets walked from the highest index down (a lower
  // bucket is drawn in FRONT), and each bucket's list is LIFO because AddPrim prepends.
  std::vector<int> order(mGroups.size());
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [this](int i, int j) {
    const uint8_t bi = mGroups[i].otBucket, bj = mGroups[j].otBucket;
    return bi != bj ? bi > bj : i > j;
  });

  // WIDE-FINAL COORDINATES (kanban #73). Every (a.x, a.y) below is the guest's own projection of the
  // player anchor: FUN_80071DFC / FUN_80072308 load the camera from scratchpad 0x1F8000F8 and project
  // through FUN_8003F7A0, which is an RTPS + store SXY2 — and native_boot re-asserts GTE CR24 = OFX =
  // nw/2 every frame under widescreen. So a.x already lands where the character is in the WIDE frame.
  // The queue's default is to centre a 4:3-authored layout, which would add the margin a second time
  // and put the digits one margin (54 px at 16:9) to the right of Tomba — exactly what the user saw.
  //
  // This declaration stays correct when the tap becomes a real port: a native producer would read the
  // player's world position and project it with the NATIVE camera, which is widened the same way, so
  // its output is wide-final too. The space is a property of "anchored to something in the world",
  // not of how the anchor is currently obtained.
  RenderQueue::Space2dScope wideFinal(c->game->activeRq(), RQ_2D_WIDE_FINAL);
  for (int i : order) {
    const UiGroupArgs& a = mGroups[i];
    lucent::debug("scorepopup", "{} bucket={:3} templ={:08X} at ({},{}) wh=({},{}) attr={:02X} clutSemi={:04X}",
                  a.sprite ? "SPR" : "FT4", a.otBucket, a.templPtr, a.x, a.y, a.wOv, a.hOv,
                  a.attrByte, a.clutSemi);
    if (a.sprite) {
      rend(c)->emitUiSprites(a.x, a.y, a.templPtr, a.dataBase, a.attrByte, a.clutSemi, RQ_OVERLAY);
    } else {
      rend(c)->emitUiFt4(a.x, a.y, a.wOv, a.hOv, a.templPtr, a.dataBase, a.attrByte, a.clutSemi,
                         RQ_OVERLAY);
    }
  }
  mGroups.clear();
}

namespace {

// FUN_80072520 — the popup entity's per-frame handler. Scope wrapper: it owns no host state of its
// own, so the guest half is the untouched gen body (the 60-frame timer, the two sub-drawers, the
// despawn and the 0x800BF83C bookkeeping all stay byte-exact).
void popupTick(Core* c) {
  ScorePopup& popup = eng(c).scorePopup;
  const bool outer = !popup.mInDraw;
  if (outer) popup.mGroups.clear();
  popup.mInDraw = true;
  gen_func_80072520(c);
  if (!outer) return;
  popup.mInDraw = false;
  popup.drawCollected();
}

}  // namespace

void ScorePopup::install() {
  static bool done = false;
  if (done) return;
  done = true;
  engine_set_override_main(0x80072520u, popupTick, gen_func_80072520);
}
