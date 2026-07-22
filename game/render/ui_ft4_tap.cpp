// UiFt4Tap — implementation. See ui_ft4_tap.h for why this address has exactly one owner.
#include "ui_ft4_tap.h"
#include "ui_group_args.h"
#include "core.h"
#include "pause_menu.h"   // PauseMenu::collect  — FUN_800346BC scope (kanban #21)
#include "score_popup.h"  // ScorePopup::collect — FUN_80072520 scope (kanban #18)

extern void gen_func_8007E1B8(Core*);
extern void engine_set_override_main(uint32_t, OverrideFn, OverrideFn);

namespace {

// FUN_8007E1B8 — the game-wide templated POLY_FT4 group emitter. Read-only: the guest half is the
// untouched gen body (packet pool / OT / scratchpad staging stay byte-exact), the native half only
// records the arguments into whichever host-side capture scope is open.
void uiFt4Tap(Core* c) {
  const UiGroupArgs a = UiGroupArgs::read(c, /*sprite=*/false);
  gen_func_8007E1B8(c);
  PauseMenu::collect(c, a);
  ScorePopup::collect(c, a);
}

}  // namespace

void UiFt4Tap::install() {
  static bool done = false;
  if (done) return;
  done = true;
  engine_set_override_main(0x8007E1B8u, uiFt4Tap, gen_func_8007E1B8);
}
