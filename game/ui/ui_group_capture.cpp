// class UiGroupCapture — implementation. See ui_group_capture.h for why the capture is shared and
// where the paint order comes from.
#include "ui_group_capture.h"
#include "core.h"
#include "game.h"
#include "game_ctx.h"        // eng(c) / rend(c)
#include "engine.h"
#include "pause_menu.h"
#include "start_page.h"
#include "render.h"          // Render::emitUiFt4 / emitUiSprites + rsub.mode.psxRender() gate
#include "render_queue.h"
#include <algorithm>
#include <numeric>

void UiGroupCapture::route(Core* c, const UiGroupArgs& a) {
  if (c->game->oracle || c->rsub.mode.psxRender()) return;   // read-only overlay gate
  Engine& e = eng(c);
  // One page controller is on the stack at a time; the group belongs to that page. Listed here, in
  // one place, so adding a page is one line and never a second override on a shared leaf.
  if (e.pauseMenu.capture.capturing()) { e.pauseMenu.capture.mGroups.push_back(a); return; }
  if (e.startPage.capture.capturing()) { e.startPage.capture.mGroups.push_back(a); return; }
  // No scope raised: this group's caller owns its own producer (the field HUD).
}

std::vector<int> UiGroupCapture::paintOrder() const {
  std::vector<int> order(mGroups.size());
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [this](int i, int j) {
    const uint8_t bi = mGroups[i].otBucket, bj = mGroups[j].otBucket;
    return bi != bj ? bi > bj : i > j;
  });
  return order;
}

void UiGroupCapture::emit(Core* c, const UiGroupArgs& a, int layer) const {
  if (a.sprite) {
    rend(c)->emitUiSprites(a.x, a.y, a.templPtr, a.dataBase, a.attrByte, a.clutSemi, layer);
  } else {
    rend(c)->emitUiFt4(a.x, a.y, a.wOv, a.hOv, a.templPtr, a.dataBase, a.attrByte, a.clutSemi,
                       layer);
  }
}
