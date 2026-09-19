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
  capture.drawAll(c, "cardmenu", RQ_OVERLAY);
}

namespace {

// FUN_8018FBCC — the card overlay's per-frame entry (the 17-state card-menu machine). Scope
// wrapper: it owns no guest state of its own, so the guest half executes dynamically.
void cardFrame(Core *c) {
  CardMenu &page = eng(c).cardMenu;
  if (page.capture.runGuestController(c, 0x8018FBCCu, __func__)) {
    page.drawCollected(c);
  }
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
