// PICTURE half of the OPTIONS screen (title front-end stage 0x801062E4 sm[0x48]==6, AND the in-game
// page raised from the START page — one page family, one producer).
//
// WHERE THE PAGE IS PRODUCED. Every element of every page is produced at its own guest EMITTER, under
// the page scope in game/ui/options_page.cpp (kanban #38): the backdrop FUN_8007FC24, the Screen-adjust
// boxes FUN_8007FCC8, and the cursor / pad-diagram groups off the two shared 2D group leaves. That is
// what makes the SAME producer serve both entry points — the earlier host reproduction of the page's
// element list (a cursor position recomputed from the label widths, an explicitly-called backdrop) was
// driven only from Render::renderTitle, so nothing drew the page in-game. This file keeps the two host
// DRAW helpers those emitters call (optionsBackdrop / optionsSolidBox) plus the ONE thing that is not
// the page's own drawing: the title chrome Demo::s6 composites UNDER the Screen-adjust page.
//
// The substrate front-end task runs the real page CONTROLLER FUN_8007B45C underneath this display pass
// every frame; that controller dispatches ONE of FIVE page builders through its bounds-checked (<5)
// jump table at 0x80016E78, selected by the u16 at task-sm+0x50 (sm = *(u32)0x1F800138):
//     0 -> FUN_8007F104  "Select Options"  (the 4-item page the Options item lands on)
//     1 -> FUN_8007F250  "Messages"        (Speed 1/2/3, Voices On/Off)
//     2 -> FUN_8007F498  "Sound"           (Speakers Stereo/Monaural, BGM + S.E. level meters)
//     3 -> FUN_8007F73C  "Screen adjust"   (Vertical/Horizontal value boxes over the LIVE title picture)
//     4 -> FUN_8007F8F8  "Controls"        (Vibration On/Off, Configuration 1/2, pad-face diagram)
// Demo::s3 seeds sm[0x50]=0 when it ENTERS s6, but leaf_8007B45C writes 1..4 on the very first Cross
// press (label L_8007B4E8 -> L_8007B540: cursor = DAT_800BF808, mem_w16(sm+0x50, cursor+1)) — so all
// five pages are reachable two presses past the title, and each needs a producer.
//
// WHAT THE PAGES DRAW (RE'd from the gen bodies: gen_func_8007F104 generated/shard_4.c:12354,
// F250 shard_5.c:13263, F498 shard_6.c:13944, F73C shard_7.c:12047, F8F8 shard_0.c:12421):
// every page is TEXT + at most three non-font element kinds, and the text is already free:
//   · TEXT / ICONS — FUN_80079374 (drawText), FUN_80079324 (drawTextSmall) and FUN_80078988 (icon
//     glyph string) are globally TAPPED (Font::drawText/drawTextSmall/iconGlyphTap dual-emit,
//     game/ui/font.cpp), so every label, every value, the shared "◉:Return ◉:Exit" footer
//     (FUN_8007F078 — icon+small-text only) AND the Sound page's BGM/S.E. level meters (10 icon
//     glyphs each, bright up to the level byte) are pushed to RQ_HUD while the substrate runs the
//     page builder. The bright/dim selection rides in the tapped glyph clut. Producers must NOT
//     re-draw text (a Font:: call would WRITE guest memory and break the read-only invariant).
//   · CURSOR — FUN_8007E998(x, Y[cursor]+4), FT4 menu template 0x98, on pages 0/1/2/4 -> the shared
//     FT4 group leaf, captured by the page scope (OptionsPage).
//   · BACKDROP — FUN_8007FC24, ONE full-screen POLY_G4 dark-blue gradient, on pages 0/1/2/4 -> ported
//     in options_page.cpp, drawn by optionsBackdrop below.
//   · BOXES — FUN_8007FCC8(x,y,w,h,flags), flat TILE rectangles, ONLY on page 3 -> ported as
//     Panel::pushDialogBackdrop, recorded by the page scope, drawn by optionsSolidBox below.
//   · PAD DIAGRAM — FUN_8007E938(.., 160, 146, 0, 225), a SPRITE template group, ONLY on page 4 ->
//     the shared SPRT group leaf, captured by the page scope.
// Page 3 is the odd one: it draws NO backdrop and composites over the live title picture, because
// Demo::s6 (game/scene/demo.cpp) special-cases sm[0x50]==3 to fire the title chrome pair
// FUN_80106824(1,1) + FUN_80106690(1) while that page is live — reproduced here as
// menuChrome() + menuItemsAndCursor(1,1).
//
// READ-ONLY: these producers read guest RAM (the options config/string table, the cursor/level bytes,
// the menu template tables) and emit host-only quads into the render queue (the caller wraps them in a
// DisplayPassGuard). No gte_op / OT / GP0 packet reading anywhere.
#include "core.h"
#include "game_ctx.h"
#include "render.h"
#include "game.h"
#include "render_queue.h"
#include <stdint.h>

static constexpr uint32_t kTaskSmPtr    = 0x1F800138u;   // scratchpad *-> current task state machine
static constexpr int      kScreenAdjustPage = 3;         // sm[0x50] value of the "Screen adjust" page

// optionsBackdrop — see render.h. The PICTURE half of FUN_8007FC24 (the guest packet itself is ported
// in game/ui/options_page.cpp): ONE full-screen POLY_G4 (op 0x38) Gouraud gradient quad covering
// (0,0)-(320,240). Vertex colors (packet bytes, verbatim): TL/TR/BL = RGB(0,0,70) dark blue, BR =
// RGB(0,0,16) — a subtle top->bottom-right darkening. Untextured (mode 3, raw 0). Absolute screen
// coords, no HUD offset (matches the guest's fixed 0..320 / 0..240 fill).
//
// LAYER: RQ_OVERLAY in the 2D-FG band, one band below the page text's RQ_HUD. The FG band matters —
// the 2D-BG band sits BEHIND the 3D world by construction (RQ_OM_2D_BG maps into (0, NATIVE_3D_MIN]),
// and the IN-GAME options page (kanban #38) is raised over a live field frame, so a BG-band backdrop
// would have the field showing straight through it. OptionsPage::drawCollected pushes this before the
// page's own chrome, so seq order inside the layer keeps it behind the cursor.
void Render::optionsBackdrop() {
  Core* c = mCore;
  // WIDESCREEN PILLARBOX: flat-black full-screen fill (equal vertex colours → STRETCHES to fill the wide
  // FB, blacking the side margins) behind the 4:3 gradient below (non-flat → CENTERS, not stretched). 4:3: no-op.
  { int xs[4] = { 0, 320, 0, 320 }, ys[4] = { 0, 0, 240, 240 }, z[4] = { 0, 0, 0, 0 };
    unsigned char k[4] = { 0, 0, 0, 0 };
    c->game->activeRq().push2dQuad(RQ_OVERLAY, /*order_2d_fg=*/1, xs, ys, z, z, k, k, k,
                                   0, 0, /*mode=*/3, /*raw=*/0, 0, 0, 0, 0, 0, 0, 0, 0, 1023, 511); }
  {
    int xs[4] = { 0, 320, 0, 320 };
    int ys[4] = { 0, 0, 240, 240 };
    int uv[4] = { 0, 0, 0, 0 };
    unsigned char rr[4] = { 0, 0, 0, 0 };
    unsigned char gg[4] = { 0, 0, 0, 0 };
    unsigned char bb[4] = { 70, 70, 70, 16 };
    c->game->activeRq().push2dQuad(RQ_OVERLAY, /*order_2d_fg=*/1, xs, ys, uv, uv, rr, gg, bb,
                                   /*tp_x=*/0, /*tp_y=*/0, /*mode=*/3, /*raw=*/0, /*clut_x=*/0, /*clut_y=*/0,
                                   /*tw_mx=*/0, /*tw_my=*/0, /*tw_ox=*/0, /*tw_oy=*/0,
                                   /*da_x0=*/0, /*da_y0=*/0, /*da_x1=*/1023, /*da_y1=*/511);
  }
}

// optionsSolidBox — see render.h. Reproduces FUN_8007FCC8(a0=x, a1=y, a2=w, a3=h, stack+16=flags):
// one 16-byte flat TILE packet (code 0x60 at +7, len 3 at +3, x/y/w/h as u16 at +8/+10/+12/+14). Its
// colour word at +4 is 70<<16 — i.e. RGB(0,0,70), the same dark blue as the backdrop gradient's bright
// vertices — when (flags & 0x7F) == 0, and 0 (black) otherwise. Untextured; RQ_OVERLAY so it lands over
// the title picture but under the page text (RQ_HUD). Screen offset applied, matching the tapped glyphs.
void Render::optionsSolidBox(int x, int y, int w, int h, uint32_t flags) {
  Core* c = mCore;
  const int ox = c->game->gpu.s_off_x, oy = c->game->gpu.s_off_y;
  const unsigned char blue = ((flags & 0x7Fu) == 0) ? 70 : 0;
  int xs[4] = { x + ox, x + w + ox, x + ox,     x + w + ox };
  int ys[4] = { y + oy, y + oy,     y + h + oy, y + h + oy };
  int uv[4] = { 0, 0, 0, 0 };
  unsigned char rr[4] = { 0, 0, 0, 0 };
  unsigned char gg[4] = { 0, 0, 0, 0 };
  unsigned char bb[4] = { blue, blue, blue, blue };
  c->game->activeRq().push2dQuad(RQ_OVERLAY, /*order_2d_fg=*/1, xs, ys, uv, uv, rr, gg, bb,
                                 /*tp_x=*/0, /*tp_y=*/0, /*mode=*/3, /*raw=*/0, /*clut_x=*/0, /*clut_y=*/0,
                                 /*tw_mx=*/0, /*tw_my=*/0, /*tw_ox=*/0, /*tw_oy=*/0,
                                 /*da_x0=*/0, /*da_y0=*/0, /*da_x1=*/1023, /*da_y1=*/511);
}

// optionsPageNative — see render.h. The page ITSELF is produced at its guest emitters (OptionsPage,
// game/ui/options_page.cpp), so what is left for the display pass is the one element that is NOT the
// page's own drawing: on page 3 "Screen adjust" the guest draws no backdrop, and Demo::s6
// (game/scene/demo.cpp) special-cases sm[0x50]==3 to fire the title chrome pair FUN_80106824(1,1) +
// FUN_80106690(1) for as long as that page is live, so the page composites over the LIVE title
// picture. Reproduced here with the existing chrome/menu producers. The dispatch on sm[0x50] mirrors
// the guest controller's own bounds-checked jump table (0..4; sm==0 = no current task = page 0).
void Render::optionsPageNative() {
  const uint32_t sm = mCore->mem_r32(kTaskSmPtr);
  const int page = sm ? (int)mCore->mem_r16(sm + 0x50u) : 0;
  if (page < 0 || page > 4)
    abortUnimplemented("DEMO OPTIONS page selector sm[0x50] out of the guest's own 0..4 range");
  if (page != kScreenAdjustPage) return;
  menuChrome();                                   // FUN_80106690(1): black fill + title logos
  menuItemsAndCursor(/*param1=*/1, /*param2=*/1); // FUN_80106824(1,1): the page-1 menu beneath
}
