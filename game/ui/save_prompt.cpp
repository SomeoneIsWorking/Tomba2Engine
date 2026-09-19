// class SavePrompt — implementation. See save_prompt.h for the RE of the prompt task, the
// measurement that identified the dropped button prompts, and why this page is a scope and nothing
// more.
#include "save_prompt.h"
#include "core.h"
#include "engine.h"
#include "game_ctx.h" // eng(c)
#include "native_override_catalog.h"
#include "render_queue.h" // RQ_OVERLAY

void SavePrompt::drawCollected(Core *c) {
  // ONE LAYER, ONE ORDER — see the ONE LIST note in ui_group_capture.h. The Cross and Circle prompts
  // and the dialog panel all carry the bucket the guest gave them, so paintOrder alone decides the
  // stacking and this producer chooses nothing.
  capture.drawAll(c, "saveprompt", RQ_OVERLAY);
}

namespace {

// FUN_800738B0 — the prompt's drawer, and the one function that emits its chrome. Scope wrapper: it
// owns no guest state of its own, so the guest half is the untouched guest-visible behavior.
void promptDraw(Core *c) {
  SavePrompt &page = eng(c).savePrompt;
  if (page.capture.runGuestController(c, 0x800738B0u, __func__)) {
    page.drawCollected(c);
  }
}

} // namespace

void SavePrompt::install() {
  static bool done = false;
  if (done) {
    return;
  }
  done = true;
  tomba::native::declareOverride(0x800738B0u, "SavePrompt::promptDraw", promptDraw);
}
