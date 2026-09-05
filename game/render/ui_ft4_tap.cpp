// UiFt4Tap — implementation. See ui_ft4_tap.h for why this address has exactly one owner.
#include "ui_ft4_tap.h"
#include "core.h"
#include "guest_call.h"
#include "native_override_catalog.h"
#include "pause_menu.h"  // PauseMenu::collect  — FUN_800346BC scope (kanban #21)
#include "score_popup.h" // ScorePopup::collect — FUN_80072520 scope (kanban #18)
#include "ui/ui_group_capture.h"
#include "ui_group_args.h"

namespace {

// FUN_8007E1B8 — the game-wide templated POLY_FT4 group emitter. Read-only: the guest half is the
// untouched guest-visible behavior (packet pool / OT / scratchpad staging stay byte-exact), the native half only
// records the arguments into whichever host-side capture scope is open.
void uiFt4Tap(Core *c) {
  const UiGroupArgs a = UiGroupArgs::read(c, /*sprite=*/false);
  psx::cpu::callOriginalToReturn(*c, 0x8007E1B8u, psx::cpu::ExecutionBudget::currentTurn(*c), __func__);
  // Two consumers, and the FIRST is a router rather than a page: UiGroupCapture::route hands the
  // group to whichever PAGE scope is raised (pause menu #21, START page #35, and any page added
  // later as one line there). Calling a single page's collect() here instead is what left the START
  // page's panel missing after the two producers were merged — its scope was never offered the group.
  UiGroupCapture::route(c, a);
  ScorePopup::collect(c, a);
}

} // namespace

void UiFt4Tap::install() {
  static bool done = false;
  if (done) {
    return;
  }
  done = true;
  tomba::native::declareOverride(0x8007E1B8u, "uiFt4Tap", uiFt4Tap);
}
