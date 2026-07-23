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
#include "screen_fade.h"     // ScreenFade — the global present-time fade this page's dim must NOT reach
#include "cfg.h"             // `pausemenu` diagnostic channel

extern void gen_func_800346BC(Core*);
extern void engine_set_override_main(uint32_t, OverrideFn, OverrideFn);


// The OT bucket the guest links its full-screen subtractive dim into (see drawCollected). Everything
// in a HIGHER bucket is painted before it and therefore dimmed.
namespace {
constexpr uint8_t kDimBucket = 132;
// The dim's level on all three channels. The guest builds it with FUN_8007E9C8(0x404040, 0, 4), which
// emits the GP0 pair `E1000040` + `62404040` — so the OT rect and the fade leaf's argument carry the
// SAME 0x40, and that identity is what lets releaseGlobalDim() below recognise its own call.
constexpr uint8_t kDimLevel = 0x40;
}  // namespace

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

// DOUBLE OWNERSHIP OF THE MENU DIM (kanban #59 — the "menu chrome too dark" fault). The guest builds
// this dim through the SHARED fade leaf FUN_8007E9C8, and ScreenFade::installLeafTap owns that leaf
// GLOBALLY: every call is mirrored into the frame-scoped ScreenFade, which the present shader (and the
// headless readback) applies to the WHOLE finished frame. That model is only correct for a fade whose
// OT rect is the topmost thing in the frame — a scene/area fade at OT slot 4 with nothing above it.
// THIS rect is not: it is linked mid-OT at kDimBucket with the outer frame, the tab labels, the item
// rows and the help panel all painted AFTER it, so on PSX it darkens only what sits beneath it. Applied
// globally instead, it darkened every pixel of the menu by a clamped 0x40 (measured at
// replays/bugs/ingame-item-menu.pad f1120: `debug fadewatch` reported mode=2 rgb=(64,64,64) on this
// path against mode=0 on the recomp leg, and the presented frame was clamp(reference - 64) everywhere).
//
// This producer draws the rect IN ITS PLACE, so it is the layer's owner and releases the global copy.
// Only the fade this page itself produced is released — a genuine scene fade running underneath the
// menu carries a different value and is left alone.
void PauseMenu::releaseGlobalDim() {
  Core* c = core;
  const ScreenFade::State s = fade(c).get();
  if (s.mode == ScreenFade::SUBTRACTIVE && s.r == kDimLevel && s.g == kDimLevel && s.b == kDimLevel)
    fade(c).set(ScreenFade::NONE, 0, 0, 0);
}

void PauseMenu::pushSubtractiveDim() {
  pushScreenQuad(kDimLevel, /*semi=*/1, /*blend=*/2);
  releaseGlobalDim();
}

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

}  // namespace

void PauseMenu::install() {
  static bool done = false;
  if (done) return;
  done = true;
  engine_set_override_main(0x800346BCu, menuTick, gen_func_800346BC);
  // The FT4 leaf FUN_8007E1B8 is owned by UiFt4Tap (game/render/ui_ft4_tap.cpp), which routes each
  // group through UiGroupCapture to whichever page scope is raised — this one included. Installing it
  // here as well is the duplicate ownership that now aborts at startup by design.
}
