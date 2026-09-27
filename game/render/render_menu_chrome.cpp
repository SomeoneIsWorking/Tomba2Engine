// game/render/render_menu_chrome.cpp — the DEMO/TITLE front-end's menu PICTURE: the shared chrome every
// page draws, the two page item rows, and the two page bodies that draw them.
//
// WHY IT MOVED OUT OF render_walk.cpp. render_walk.cpp is the field and world WALK — entity lists,
// terrain, the scene table, objects, backdrop — and it sits at the project's 1,200-line cap, so it
// cannot absorb a paragraph without a split. The title menu shares none of that: it reads the front-end
// state at 0x1F800138 and emits 2D template quads through the shared UI emitters, and it runs from
// Render::renderTitle's s48==2 / s48==3 arms rather than from the field walk. Extracting it is a placement
// correction, not a refactor for its own sake: the two have no shared state beyond Render's own members.
//
// One rule worth reading before changing anything here: this file draws at RQ_BACKGROUND for the chrome
// and RQ_OVERLAY for the items and cursor, so the backdrop is behind the row text by construction. The
// in-game pause/item menu is a DIFFERENT page with a different owner (game/ui/pause_menu.cpp) and its own
// full-screen opaque backdrop; do not reach for these wrappers to draw it.
#include "core.h"
#include "game.h"
#include "producer_scope.h" // ProducerScope — graphics-producer DB, native leg
#include "render.h"
#include "render_queue.h"

// emitMenuFt4 / emitMenuSprites — the MENU-specialized wrappers over the general UI template-group
// cores (Render::emitUiFt4 / emitUiSprites, game/render/field_hud.cpp, which own the guest emitters —
// added for the field HUD, kanban #13). Menu fixed points: template ptr table 0x80017334, FT4 data base 0x80158000,
// sprite data base = *(0x800ECF58). Decoder validated field-for-field against the title's known-good
// quads (docs/findings/render.md '#2b') and the Controls pad diagram (template 225).
void Render::emitMenuFt4(int anchorX, int anchorY, uint32_t templateIdx, uint32_t attr, int layer) {
  Core *c = mCore;
  emitUiFt4(anchorX, anchorY, 0, 0, c->mem_r32(0x80017334u + templateIdx * 4u), 0x80158000u, (uint8_t)attr, 0, layer);
}

void Render::emitMenuSprites(int anchorX, int anchorY, uint32_t templateIdx, uint32_t attr, int layer) {
  Core *c = mCore;
  emitUiSprites(
      anchorX, anchorY, c->mem_r32(0x80017334u + templateIdx * 4u), c->mem_r32(0x800ECF58u), (uint8_t)attr, 0, layer);
}

// menuChrome — see render.h. The black backdrop + the 2 logo sprites (FUN_80106690), shared by every
// front-end menu page. The logos are op-0x65 raw sprites (fixed layout, decoded packet constants).
void Render::menuChrome() {
  Core *c = mCore;
  // Producer DB, native leg. Keyed on the guest emitter this reimplements (codemap --addr 0x80106690
  // -> Render::menuChrome). Found by PSXPORT_DEBUG=unscoped, which names the CALL SITE of every prim that
  // arrives with no producer declared — this one reached the queue through a SHARED emitter
  // (emitRecordQuad / emitUiFt4 / emitUiSprites / worldLineDraw), so the scope belongs here at the
  // producer, never on the emitter, which would shadow every one of its callers.
  ProducerScope menuChromeScope(&c->rsub.producerScope, 0x80106690u, "menuChrome");
  const int ox = c->game->gpu.s_off_x, oy = c->game->gpu.s_off_y;
  {
    int xs[4] = {0, 320, 0, 320}, ys[4] = {0, 0, 240, 240}, z[4] = {0, 0, 0, 0};
    unsigned char k[4] = {0, 0, 0, 0};
    c->game->activeRq().push2dQuad(
        RQ_BACKGROUND, 0, xs, ys, z, z, k, k, k, 0, 0, /*mode=*/3, /*raw=*/0, 0, 0, 0, 0, 0, 0, 0, 0, 1023, 511);
  }
  titleWideMargins();
  auto logo = [&](int x, int w, int tp_x) { // tpage 0x9A(640)/0x9C(768), 8bpp
    int xs[4] = {x + ox, x + w + ox, x + ox, x + w + ox}, ys[4] = {-8 + oy, -8 + oy, 232 + oy, 232 + oy};
    int us[4] = {0, w, 0, w}, vs[4] = {0, 0, 240, 240};
    unsigned char cc[4] = {0x80, 0x80, 0x80, 0x80};
    c->game->activeRq().push2dQuad(RQ_BACKGROUND,
                                   1,
                                   xs,
                                   ys,
                                   us,
                                   vs,
                                   cc,
                                   cc,
                                   cc,
                                   tp_x,
                                   256,
                                   /*mode=*/1,
                                   /*raw=*/1,
                                   640,
                                   511,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   1023,
                                   511);
  };
  logo(0, 256, 640);
  logo(256, 64, 768);
}

// menuItemsAndCursor — see render.h. Reproduces FUN_80106824(param1, param2): the cursor (template 0x98
// at the game's cursor-X table @0x80107704) then the two item text-images (page-0 {0x8e,0x8f} title, or
// page-1 {0x90,0x91} s3), the param2-selected item RAW/bright and the other modulated 0x50/dim.
void Render::menuItemsAndCursor(int param1, int param2) {
  Core *c = mCore;
  // Producer DB, native leg. Keyed on the guest menu emitter this reimplements (0x80106824, ov_demo
  // overlay-resident with no overlay collision). Its prims arrived through the SHARED emitters
  // emitUiFt4 / emitMenuFt4, which must not be scoped themselves.
  ProducerScope menuScope(&c->rsub.producerScope, 0x80106824u, "menuItemsAndCursor");
  static const uint32_t TMPL[2][2] = {{0x8Eu, 0x8Fu}, {0x90u, 0x91u}};
  const uint32_t t0 = TMPL[param1 & 1][0], t1 = TMPL[param1 & 1][1];
  const uint32_t a0 = (param2 == 0) ? 0u : 0x50u, a1 = (param2 == 0) ? 0x50u : 0u;
  const int cx = (int16_t)c->mem_r16(0x80107704u + (uint32_t)(param2 * 2 + param1 * 4)); // cursor anchor X
  // Draw items first, cursor last: where a wide item (page-1 item0 at x43) overlaps the cursor (x32..48),
  // the reference draws the cursor ON TOP (verified by pixel-diff: cursor-last -> RMSE ~0; cursor-first
  // left a 15px seam at the overlap). The guest OT links cursor after the items but is walked cursor-first.
  emitMenuFt4(90, 180, t0, a0, RQ_OVERLAY);     // item 0 anchor (90,180)
  emitMenuFt4(230, 180, t1, a1, RQ_OVERLAY);    // item 1 anchor (230,180)
  emitMenuFt4(cx, 0xB0, 0x98u, 0u, RQ_OVERLAY); // cursor (template 0x98, raw), y anchor 176
}

// titleNative — see render.h. Read-only producer for the DEMO/title front-end page 0 (sm[0x48]==2, the
// New/Load menu). Chrome (backdrop + logos) + the page-0 menu, entirely data-driven off the guest menu
// templates (no hand-decoded constants). Selection sel = sm[0x68] (Demo::s2SubMachine writes it); the
// guest calls FUN_80106824(0, sm[0x68]!=0).
void Render::titleNative() {
  Core *c = mCore;
  menuChrome();
  uint32_t sm = c->mem_r32(0x1F800138u);
  int sel = sm ? c->mem_r8(sm + 0x68u) : 0;
  menuItemsAndCursor(0, (sel != 0) ? 1 : 0);
}

// s3MenuNative — see render.h. The page-1 menu (sm[0x48]==3, reached by confirming New Game). Same chrome,
// page-1 templates; the guest (Demo::s3 -> s3SubMachine) calls FUN_80106824(1, sm[0x68]!=2).
void Render::s3MenuNative() {
  Core *c = mCore;
  menuChrome();
  uint32_t sm = c->mem_r32(0x1F800138u);
  int s68 = sm ? c->mem_r8(sm + 0x68u) : 2;
  menuItemsAndCursor(1, (s68 != 2) ? 1 : 0);
}
