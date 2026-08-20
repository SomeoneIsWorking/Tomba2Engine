// class StartPage — implementation. See start_page.h for the RE of FUN_8007EAE4, the guest-data
// measurement that identified the missing layer, and why this page has no backdrop and no dim.
#include "start_page.h"
#include "cfg.h" // `startpage` diagnostic channel
#include "core.h"
#include "engine.h"
#include "game_ctx.h"     // eng(c)
#include "render_queue.h" // RQ_OVERLAY

extern void gen_func_8007EAE4(Core *);
extern void engine_set_override_main(uint32_t, OverrideFn, OverrideFn);

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
// so the guest half is the untouched gen body.
void pageDraw(Core *c) {
  StartPage &page = eng(c).startPage;
  const bool outer = !page.capture.capturing();
  if (outer) {
    page.capture.clear();
  }
  page.capture.begin();
  gen_func_8007EAE4(c); // byte-exact: the option strings + the chrome's packet emission
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
  engine_set_override_main(0x8007EAE4u, pageDraw, gen_func_8007EAE4);
}
