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
  capture.drawAll(c, "startpage", RQ_OVERLAY);
}

namespace {

// FUN_8007EAE4 — the in-game START page drawer. Scope wrapper: it owns no guest state of its own,
// so the guest half is the untouched guest-visible behavior.
void pageDraw(Core *c) {
  StartPage &page = eng(c).startPage;
  // byte-exact: the option strings + the chrome's packet emission
  if (page.capture.runGuestController(c, 0x8007EAE4u, __func__)) {
    page.drawCollected(c);
  }
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
