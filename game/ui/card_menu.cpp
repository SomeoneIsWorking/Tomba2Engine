// class CardMenu — implementation. See card_menu.h for the RE of the card overlay's frame entry,
// the measurement that identified the missing layers, and why this page is a scope and nothing more.
#include "card_menu.h"
#include "core.h"
#include "engine.h"
#include "game_ctx.h" // eng(c)
#include "guest_call.h"
#include "native_override_catalog.h"
#include "render_queue.h" // RQ_OVERLAY
#include <lucent/log.h>

void CardMenu::drawCollected(Core *c) {
  // ONE LAYER, ONE ORDER — see the ONE LIST note in ui_group_capture.h. Everything the card menu
  // links into its ordering table (the backdrop grid, the "Save" badge, the button prompts, the slot
  // save icon, and the header/slot PANELS) is in `capture` with the bucket the guest gave it, so
  // paintOrder alone decides the stacking and this producer chooses nothing. Measured on the
  // save-slot page: the backdrop grid is the only thing in bucket 6 and lands behind everything;
  // badge, prompts, save icon and panels share bucket 5 and stack by the guest's LIFO within it.
  int drawn = 0, panels = 0;
  for (int i : capture.paintOrder()) {
    const PageChromeItem &it = capture.mItems[i];
    lucent::debug("cardmenu",
                  "{} bucket={:3} templ={:08X} at ({},{}) attr={:02X} clutSemi={:04X}",
                  it.kind != PageChromeItem::Kind::Group ? "PANEL"
                  : it.group.sprite                      ? "SPR"
                                                         : "FT4",
                  it.otBucket,
                  it.group.templPtr,
                  it.group.x,
                  it.group.y,
                  it.group.attrByte,
                  it.group.clutSemi);
    capture.emit(c, it, RQ_OVERLAY);
    drawn++;
    panels += (it.kind != PageChromeItem::Kind::Group);
  }
  // The denominator, not just the hits: a card screen that filed NOTHING is the state this producer
  // exists to fix, and it must not read the same as one that drew.
  lucent::debug("cardmenu",
                "frame drew {} item(s) — {} panel(s), {} group(s){}",
                drawn,
                panels,
                drawn - panels,
                drawn ? "" : " — NOTHING was filed under the card scope this frame");
  capture.clear();
}

namespace {

// FUN_8018FBCC — the card overlay's per-frame entry (the 17-state card-menu machine). Scope
// wrapper: it owns no guest state of its own, so the guest half executes dynamically.
void cardFrame(Core *c) {
  CardMenu &page = eng(c).cardMenu;
  const bool outer = !page.capture.capturing();
  if (outer) {
    page.capture.clear();
  }
  page.capture.begin();
  psx::cpu::callOriginalToReturn(*c, 0x8018FBCCu, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
  if (!outer) {
    return;
  }
  page.capture.end();
  page.drawCollected(c);
}

} // namespace

void CardMenu::install() {
  static bool done = false;
  if (done) {
    return;
  }
  done = true;
  tomba::native::declareOverride(0x8018FBCCu, "CardMenu::cardFrame", cardFrame);
}
