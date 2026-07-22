// class PauseMenu — implementation. See pause_menu.h for the architecture note, the measurement
// that identified the missing layer, and ui_group_capture.h for why paint order comes from the
// guest's OT bucket.
#include "pause_menu.h"
#include "core.h"
#include "game.h"
#include "game_ctx.h"        // eng(c) / rend(c)
#include "engine.h"
#include "render.h"          // Render::emitUiFt4 / emitUiSprites
#include "render_queue.h"    // RQ_OVERLAY
#include "cfg.h"             // `pausemenu` diagnostic channel

extern void gen_func_800346BC(Core*);
extern void gen_func_8007E1B8(Core*);
extern void engine_set_override_main(uint32_t, OverrideFn, OverrideFn);

// The OT bucket the guest links its full-screen subtractive dim into (see drawCollected). Everything
// in a HIGHER bucket is painted before it and therefore dimmed.
namespace { constexpr uint8_t kDimBucket = 132; }

// One full-screen quad, shared by the menu's opaque black backdrop and its subtractive dim.
void PauseMenu::pushScreenQuad(unsigned char level, int semi, int blend) {
  Core* c = core;
  const int ox = c->game->gpu.s_off_x, oy = c->game->gpu.s_off_y;
  int xs[4] = { ox, 320 + ox, ox, 320 + ox };
  int ys[4] = { oy, oy, 240 + oy, 240 + oy };
  int z[4] = { 0, 0, 0, 0 };
  unsigned char cc[4] = { level, level, level, level };
  c->game->activeRq().emitOrQueue(c, /*capture=*/1, RQ_OVERLAY, RQ_OM_2D_FG, /*nv=*/4, semi,
                                  /*raw=*/0, xs, ys, nullptr, nullptr, z, z, cc, cc, cc,
                                  /*depth=*/nullptr, /*mode=*/3, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 1023, 511, blend);
}

void PauseMenu::pushSubtractiveDim() { pushScreenQuad(0x40, /*semi=*/1, /*blend=*/2); }

namespace { constexpr uint32_t kGuestFrameCounter = 0x1F80017Cu; }

bool PauseMenu::upThisFrame() const {
  return core && mLastDrawFrame == core->mem_r32(kGuestFrameCounter);
}

void PauseMenu::drawCollected() {
  Core* c = core;
  if (capture.empty()) return;
  mLastDrawFrame = c->mem_r32(kGuestFrameCounter);

  // The menu's own FULL-SCREEN BLACK BACKDROP, painted before every other menu element. The guest
  // emits it as one GP0 0x60 tile and it is the FIRST packet in the ordering-table walk while the
  // menu is up (`debug otattr` index [1] of 421, pool 0x800C20B0 = `60000000 00000000 00F00140`:
  // opaque, colour 0x000000, (0,0), 320x240 — read back with the REPL `rw` on the psx_render leg,
  // 2026-07-22). It is what makes the real game's menu sit on solid black rather than on the field:
  // while the pause menu is up the guest submits NO world geometry at all (all 421 packets that
  // frame come from the menu controller), so without it pc_render's field pass showed through
  // around the panel edges.
  pushScreenQuad(0x00, /*semi=*/0, /*blend=*/0);

  bool dimDone = false;
  for (int i : capture.paintOrder()) {
    const UiGroupArgs& a = capture.mGroups[i];
    // The menu's SUBTRACTIVE full-screen dim, linked at OT bucket kDimBucket ahead of that bucket's
    // own groups — so it paints over everything in higher buckets (the drop shadow, the outer frame,
    // the tab labels and the rule under them) and under everything below (the panel interior, the
    // item icons, the divider). Guest packet, read back on the psx_render leg 2026-07-22: a GP0 0xE1
    // draw-mode word `E1000040` (blend bits = 2, i.e. B - F) followed by `62404040 00000000 00F00140`
    // — colour 0x404040 over the whole 320x240 screen. Without it every element above the panel came
    // out exactly 0x40 too bright on all three channels (measured at the tab glyphs and the rule).
    if (!dimDone && a.otBucket < kDimBucket) { dimDone = true; pushSubtractiveDim(); }
    cfg_logf("pausemenu", "%s bucket=%3u templ=%08X at (%d,%d) wh=(%d,%d) attr=%02X clutSemi=%04X",
             a.sprite ? "SPR" : "FT4", a.otBucket, a.templPtr, a.x, a.y, a.wOv, a.hOv,
             a.attrByte, a.clutSemi);
    capture.emit(c, a, RQ_OVERLAY);
  }
  if (!dimDone) pushSubtractiveDim();   // every group sat at or above kDimBucket
  capture.clear();
}

namespace {

// FUN_800346BC — the pause/item-menu controller. Scope wrapper: it owns no guest state of its own,
// so the guest half is the untouched gen body.
void menuTick(Core* c) {
  PauseMenu& menu = eng(c).pauseMenu;
  const bool outer = !menu.capture.capturing();
  if (outer) menu.capture.clear();
  menu.capture.begin();
  gen_func_800346BC(c);          // byte-exact: the whole menu state machine + its packet emission
  if (!outer) return;
  menu.capture.end();
  menu.drawCollected();
}

// FUN_8007E1B8 — the game-wide templated POLY_FT4 group emitter. This file is its ONE owner; every
// page that paints through it gets its groups routed by UiGroupCapture::route.
void uiFt4Tap(Core* c) {
  const UiGroupArgs a = UiGroupCapture::readArgs(c, /*sprite=*/false);
  gen_func_8007E1B8(c);          // byte-exact guest packet pool / OT / scratchpad staging
  UiGroupCapture::route(c, a);
}

}  // namespace

void PauseMenu::install() {
  static bool done = false;
  if (done) return;
  done = true;
  engine_set_override_main(0x800346BCu, menuTick, gen_func_800346BC);
  engine_set_override_main(0x8007E1B8u, uiFt4Tap, gen_func_8007E1B8);
}
