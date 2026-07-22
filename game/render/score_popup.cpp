// class ScorePopup — implementation. See score_popup.h for the RE, the measurement that identified
// the missing layer, and why paint order comes from the guest's OT bucket.
#include "score_popup.h"
#include "core.h"
#include "game.h"
#include "game_ctx.h"        // eng(c) / rend(c)
#include "engine.h"
#include "render.h"          // Render::emitUiFt4 / emitUiSprites + rsub.mode.psxRender() gate
#include "render_queue.h"    // RQ_OVERLAY
#include "cfg.h"             // `scorepopup` diagnostic channel
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

  for (int i : order) {
    const UiGroupArgs& a = mGroups[i];
    cfg_logf("scorepopup", "%s bucket=%3u templ=%08X at (%d,%d) wh=(%d,%d) attr=%02X clutSemi=%04X",
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
