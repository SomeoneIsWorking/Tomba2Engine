// class StartPage — native display producer for the IN-GAME START PAGE (kanban #35): the little
// framed panel holding "Options / Load data / Quit game" that the START button raises over the
// field.
//
// THE GUEST SIDE. FUN_8007EAE4 is the whole page drawer, reached under the GAME-overlay page
// dispatcher FUN_8010810C (Engine::submitPage810c). Its body (gen_func_8007EAE4, shard_7) does two
// things and nothing else:
//   * three option strings — the labels at 0x800A2854 / +4 / +8, drawn through the font emitters
//     FUN_800793C4 / FUN_80079374, with the highlight picked by the SELECTED-ITEM byte at
//     0x800BF808. Those are already produced natively by the global Font taps (game/ui/font.cpp),
//     which is why the TEXT rendered even while the panel did not.
//   * the panel chrome — a nine-iteration loop that stages a placement record and an attribute
//     record on its own guest stack frame (attrs+0 = 0, attrs+1 = OT bucket 1, attrs+2 = 0x8000 =
//     the semi-transparent flag) and hands each to the shared POLY_FT4 group emitter FUN_8007E1B8,
//     plus the cursor sprite via FUN_8007E998 -> FUN_8007E8DC -> the same emitter. Those nine
//     pieces are the panel body and its frame ornaments; they had NO native producer.
//
// THE MEASUREMENT (2026-07-22, in-game START page raised on the seaside field at frame 388):
//   * psx_render leg: `PSXPORT_PRIMAT="200,132,388"` reports the panel body —
//     `op=2F is3d=0 semi=1 tex=1 raw=1 mode=0 tp=(384,0) clut=(496,216) uv0=(241,46)
//      col=(198,254,202) bbox=(106,81)-(208,145)`.
//   * pc_render leg: the SAME probe reports no such prim in the render queue at all — only field
//     geometry and the glyphs. The layer was absent, not mis-blended.
//   * `debug otattr` on that frame: 12 packets fn=0x8007E2F8 (inside FUN_8007E1B8) caller=
//     0x8007EAE4 — 11 semi FT4 groups plus the op-0x2D cursor — against 26 glyph/texpage packets
//     emitted directly by 0x8007EAE4.
//
// THE PRODUCER = A SCOPE, NOTHING MORE. FUN_8007E1B8 and FUN_8007E6DC are already owned and already
// tapped (pause_menu.cpp / ui_sprite.cpp) — tapping them again would be a second owner on one guest
// address, the invisible dual-ownership bug that broke the dialog box (kanban #28). So this page
// only needs its own UiGroupCapture scope around FUN_8007EAE4: raise it, run the untouched gen
// body, lower it, then draw what the shared taps filed under it.
//
// NO BACKDROP, NO DIM — deliberately unlike PauseMenu. That frame contains ZERO GP0 0x60/0x62
// full-screen tiles and no 0x404040 subtractive dim; the field keeps rendering underneath (725
// world packets from 0x8003F9A8 in the same otattr dump) and the panel composites over it with its
// own semi bit (semi=1, mode 0 = 0.5*B + 0.5*F). Copying PauseMenu::drawCollected wholesale would
// black out a field the page is supposed to show through.
//
// The field HUD is NOT stood down here (unlike the pause menu, whose frame is menu-only): the guest
// keeps submitting the world and its HUD dispatcher while this page is up.
#pragma once
#include "ui_group_capture.h"
class Core;

class StartPage {
public:
  // The page's chrome groups, captured off the shared 2D group emitters for as long as the drawer
  // FUN_8007EAE4 is on the stack (see pageDraw). Per-Core, so SBS's two cores cannot see each
  // other's scope.
  UiGroupCapture capture;

  // install(): registers the FUN_8007EAE4 scope wrapper on the main-EXE table. Idempotent; called
  // from games_tomba2_init alongside the other *_install() wirings.
  static void install();

  // drawCollected: push the captured groups in the guest's paint order (descending OT bucket, LIFO
  // within a bucket — which puts the cursor, emitted first, on top) at RQ_OVERLAY, one band below
  // the RQ_HUD glyphs the Font taps produce.
  void drawCollected(Core* c);
};
