// class StartPage — implementation. See start_page.h for the RE of FUN_8007EAE4, the guest-data
// measurement that identified the missing layer, and why this page has no backdrop and no dim.
#include "start_page.h"
#include "cfg.h" // `startpage` diagnostic channel
#include "core.h"
#include "engine.h"
#include "game_ctx.h" // eng(c)
#include "guest_call.h"
#include "native_override_catalog.h"
#include "render_queue.h" // RQ_OVERLAY

void StartPage::drawCollected(Core *c) {
  for (int i : capture.paintOrder()) {
    const PageChromeItem &it = capture.mItems[i];
    cfg_logf("startpage",
             "%s bucket=%3u templ=%08X at (%d,%d) wh=(%d,%d) attr=%02X clutSemi=%04X",
             it.kind != PageChromeItem::Kind::Group ? "PANEL"
             : it.group.sprite                      ? "SPR"
                                                    : "FT4",
             it.otBucket,
             it.group.templPtr,
             it.group.x,
             it.group.y,
             it.group.wOv,
             it.group.hOv,
             it.group.attrByte,
             it.group.clutSemi);
    capture.emit(c, it, RQ_OVERLAY);
  }
  capture.clear();
}

namespace {

// FUN_8007EAE4 — the in-game START page drawer. Scope wrapper: it owns no guest state of its own,
// so the guest half is the untouched guest-visible behavior.
void pageDraw(Core *c) {
  StartPage &page = eng(c).startPage;
  const bool outer = !page.capture.capturing();
  if (outer) {
    page.capture.clear();
  }
  page.capture.begin();
  psx::cpu::callOriginalToReturn(*c,
                                 0x8007EAE4u,
                                 psx::cpu::ExecutionBudget::currentTurn(*c),
                                 __func__); // byte-exact: the option strings + the chrome's packet emission
  if (!outer) {
    return;
  }
  page.capture.end();
  page.drawCollected(c);
}

} // namespace

void StartPage::install() {
  static bool done = false;
  if (done) {
    return;
  }
  done = true;
  tomba::native::declareOverride(0x8007EAE4u, "pageDraw", pageDraw);
}
