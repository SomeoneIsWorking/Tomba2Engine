// NATIVE pc_render producers for the DEMO/title front-end OPTIONS screen (stage 0x801062E4, sm[0x48]==6).
//
// Reached from Render::renderTitle's s48==6 branch: title menu -> "Options" -> Confirm. The substrate
// front-end task runs the real page CONTROLLER FUN_8007B45C underneath this display pass every frame;
// that controller dispatches ONE of FIVE page builders through its bounds-checked (<5) jump table at
// 0x80016E78, selected by the u16 at task-sm+0x50 (sm = *(u32)0x1F800138):
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
//   · CURSOR — FUN_8007E998(x, Y[cursor]+4), FT4 menu template 0x98, on pages 0/1/2/4. Same sprite
//     the title menu uses; emitted through the proven Render::emitMenuFt4 template decoder.
//   · BACKDROP — FUN_8007FC24, ONE full-screen POLY_G4 dark-blue gradient, on pages 0/1/2/4.
//   · BOXES — FUN_8007FCC8(x,y,w,h,flags), flat TILE rectangles, ONLY on page 3.
//   · PAD DIAGRAM — FUN_8007E938(.., 160, 146, 0, 225), a SPRITE template group, ONLY on page 4.
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

// OPTIONS config/string table base (gen: r16 = 32778<<16 + 10324). Every page reads its labels from
// fixed offsets in this table; the layout, per page builder:
//   +48 page0 header   +52..+64 page0 items      +68/+80 page1 items    +72/+76 page1 speed labels
//   +56 page2 header   +92/+104/+116 page2 items +108/+112 page2 meter labels
//   +60 page3 header   +120/+124 page3 items
//   +64 page4 header   +128/+140 page4 items     +132..+156 page4 diagram labels
static constexpr uint32_t kOptCfgBase   = 0x800A2854u;
static constexpr uint32_t kMenuCursorB  = 0x800BF808u;   // shared menu cursor byte (u8 selected item)
static constexpr uint32_t kTaskSmPtr    = 0x1F800138u;   // scratchpad *-> current task state machine
static constexpr uint32_t kCursorTmpl   = 0x98u;         // FT4 cursor template index (== title cursor)
static constexpr uint32_t kPadDiagTmpl  = 225u;          // sprite template group: the pad-face diagram
static constexpr int      kSubPageCurX  = 44;            // cursor X on every sub-page (pages 1/2/4)

// guestStrLen — see render.h. Read-only NUL scan (the host twin of FUN_80079528 / FUN_8009A600).
int Render::guestStrLen(uint32_t str) {
  int n = 0;
  while (str && mCore->mem_r8(str + (uint32_t)n) != 0) n++;
  return n;
}

// optionsBackdrop — see render.h. Reproduces FUN_8007FC24: ONE full-screen POLY_G4 (op 0x38) Gouraud
// gradient quad covering (0,0)-(320,240). Vertex colors (packet bytes, verbatim): TL/TR/BL = RGB(0,0,70)
// dark blue, BR = RGB(0,0,16) — a subtle top->bottom-right darkening. Untextured (mode 3, raw 0),
// RQ_BACKGROUND so it sits behind the page text (RQ_HUD) and cursor (RQ_OVERLAY). Absolute screen
// coords, no HUD offset (matches the guest's fixed 0..320 / 0..240 fill).
void Render::optionsBackdrop() {
  Core* c = mCore;
  // WIDESCREEN PILLARBOX: flat-black full-screen fill (equal vertex colours → STRETCHES to fill the wide
  // FB, blacking the side margins) behind the 4:3 gradient below (non-flat → CENTERS, not stretched). 4:3: no-op.
  { int xs[4] = { 0, 320, 0, 320 }, ys[4] = { 0, 0, 240, 240 }, z[4] = { 0, 0, 0, 0 };
    unsigned char k[4] = { 0, 0, 0, 0 };
    c->game->activeRq().push2dQuad(RQ_BACKGROUND, /*order_2d_fg=*/0, xs, ys, z, z, k, k, k,
                                   0, 0, /*mode=*/3, /*raw=*/0, 0, 0, 0, 0, 0, 0, 0, 0, 1023, 511); }
  {
    int xs[4] = { 0, 320, 0, 320 };
    int ys[4] = { 0, 0, 240, 240 };
    int uv[4] = { 0, 0, 0, 0 };
    unsigned char rr[4] = { 0, 0, 0, 0 };
    unsigned char gg[4] = { 0, 0, 0, 0 };
    unsigned char bb[4] = { 70, 70, 70, 16 };
    c->game->activeRq().push2dQuad(RQ_BACKGROUND, /*order_2d_fg=*/0, xs, ys, uv, uv, rr, gg, bb,
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

// optionsPageNative — see render.h. Dispatches the live page exactly like the guest controller's
// bounds-checked jump table: sm[0x50] in 0..4, anything else is impossible by construction (the
// controller range-checks before jumping). sm==0 (no current task) can only mean page 0.
void Render::optionsPageNative() {
  const uint32_t sm = mCore->mem_r32(kTaskSmPtr);
  const int page = sm ? (int)mCore->mem_r16(sm + 0x50u) : 0;
  switch (page) {
    case 0: optionsSelectPage();       return;
    case 1: optionsMessagesPage();     return;
    case 2: optionsSoundPage();        return;
    case 3: optionsScreenAdjustPage(); return;
    case 4: optionsControlsPage();     return;
    default: break;
  }
  abortUnimplemented("DEMO OPTIONS page selector sm[0x50] out of the guest's own 0..4 range");
}

// optionsSelectPage — see render.h. Page 0 "Select Options" (FUN_8007F104):
//   header icon-string FUN_80078988(38,200)              -> icon TAP
//   title label        FUN_80079324(50,200,6, cfg+160)   -> font TAP
//   static header      FUN_80079374(32,32,  cfg+48)      -> font TAP
//   4 option labels    FUN_80079374, y = 76+24*i, cfg+52+4*i, size 8 on the cursor row -> font TAP
//   cursor sprite      FUN_8007E998(cursorX, 80+24*cursor)  -> HERE (FT4 template 0x98)
//   backdrop           FUN_8007FC24                          -> HERE
void Render::optionsSelectPage() {
  Core* c = mCore;
  optionsBackdrop();
  // CURSOR X: reproduce FUN_800793C4's return value. It measures each of the 4 option strings, lays each
  // out at X[i] = 160 - (strlen(str[i])>>1)*8 (centered around x=160, 8px/char), tracks the MINIMUM X
  // (leftmost item) and returns that minimum - 16 (the cursor sits 16px left of the leftmost option).
  // r19 starts at 160, so an empty/absent string can't push the min above 160. Closed form below.
  int maxHalf = 0;
  for (int i = 0; i < 4; i++) {
    const int half = guestStrLen(c->mem_r32(kOptCfgBase + 52u + (uint32_t)i * 4u)) >> 1;
    if (half > maxHalf) maxHalf = half;
  }
  const int cursorX = 160 - maxHalf * 8 - 16;
  int cursor = (int)c->mem_r8(kMenuCursorB);
  if (cursor < 0 || cursor > 3) cursor = 0;              // host-only clamp (valid range is 0..3)
  emitMenuFt4(cursorX, 76 + 24 * cursor + 4, kCursorTmpl, /*attr=*/0u, RQ_OVERLAY);
}

// optionsMessagesPage — see render.h. Page 1 "Messages" (FUN_8007F250): header + 2 option rows
// (Message speed / Voices) at x=64, y = {96,128}, their value labels, the speed digits and the footer —
// all font/icon taps. The only non-font elements are the cursor at x=44, y = row+4, and the backdrop.
void Render::optionsMessagesPage() {
  static const int ROW_Y[2] = { 96, 128 };               // gen: sp+40 = {0x60, 0x80}
  optionsBackdrop();
  const int cursor = (int)mCore->mem_r8(kMenuCursorB) % 2;   // host-only clamp (2 rows)
  emitMenuFt4(kSubPageCurX, ROW_Y[cursor] + 4, kCursorTmpl, /*attr=*/0u, RQ_OVERLAY);
}

// optionsSoundPage — see render.h. Page 2 "Sound" (FUN_8007F498): header + 3 option rows (Speakers /
// BGM / S.E.) at x=64, y = {80,112,144}. The BGM and S.E. LEVEL METERS are NOT geometry — the guest
// draws each as 10 icon-glyph strings (FUN_80078988 at x = 180+8*k, y = 116 / 148, size 3 up to the
// level byte and 0 beyond it), so the whole meter arrives through the icon tap. Non-font: cursor +
// backdrop only.
void Render::optionsSoundPage() {
  static const int ROW_Y[3] = { 80, 112, 144 };          // gen: sp+40 = {0x50, 0x70, 0x90}
  optionsBackdrop();
  const int cursor = (int)mCore->mem_r8(kMenuCursorB) % 3;   // host-only clamp (3 rows)
  emitMenuFt4(kSubPageCurX, ROW_Y[cursor] + 4, kCursorTmpl, /*attr=*/0u, RQ_OVERLAY);
}

// optionsScreenAdjustPage — see render.h. Page 3 "Screen adjust" (FUN_8007F73C). The odd page:
//   · NO backdrop call. Instead Demo::s6 fires the title chrome pair FUN_80106824(1,1) +
//     FUN_80106690(1) for as long as sm[0x50]==3, so the page composites over the LIVE title picture
//     (page-1 menu). Reproduced with the existing chrome/menu producers.
//   · NO cursor sprite — the selected row is shown by the text colour alone (size 3 vs 0).
//   · THREE flat boxes via FUN_8007FCC8, the only geometry:
//       (a) header box   (40, 44, strlen(cfg+60)*8 + 16, 24)      flags 0 -> dark blue
//       (b) per row i    x = 144 - w, y = ROW_Y[i] - 4, w + 48 wide, 24 high, w = strlen(cfg+120+4i)*8
//       (c) footer box   (100, 193, 192, 21)                      flags 1 -> black
//     The row labels are right-aligned at x = 152 - w and the value text (sprintf'd from the guest's
//     live vertical/horizontal offsets at 0x800F7E72 / 0x800F7E70) at x=168 — both font taps.
void Render::optionsScreenAdjustPage() {
  static const int ROW_Y[2] = { 92, 132 };               // gen: sp+40 = {0x5C, 0x84}
  menuChrome();                                          // FUN_80106690(1): black fill + title logos
  menuItemsAndCursor(/*param1=*/1, /*param2=*/1);        // FUN_80106824(1,1): the page-1 menu beneath
  optionsSolidBox(40, 44, guestStrLen(mCore->mem_r32(kOptCfgBase + 60u)) * 8 + 16, 24, /*flags=*/0u);
  for (int i = 0; i < 2; i++) {
    const int w = guestStrLen(mCore->mem_r32(kOptCfgBase + 120u + (uint32_t)i * 4u)) * 8;
    optionsSolidBox(144 - w, ROW_Y[i] - 4, w + 48, 24, /*flags=*/0u);
  }
  optionsSolidBox(100, 193, 192, 21, /*flags=*/1u);      // black strip under the footer icons
}

// optionsControlsPage — see render.h. Page 4 "Controls" (FUN_8007F8F8): header + 2 option rows
// (Vibration / Configuration) at x=64, y = {64,88}, plus the pad-face diagram's leader labels (whose
// text order swaps on the config byte at 0x800FB166) — all font taps. Non-font: the cursor at x=44,
// the PAD DIAGRAM sprite group (FUN_8007E938 -> FUN_8007E6DC, template 225 anchored at 160,146), and
// the backdrop.
void Render::optionsControlsPage() {
  static const int ROW_Y[2] = { 64, 88 };                // gen: sp+40 = {0x40, 0x58}
  optionsBackdrop();
  const int cursor = (int)mCore->mem_r8(kMenuCursorB) % 2;   // host-only clamp (2 rows)
  emitMenuFt4(kSubPageCurX, ROW_Y[cursor] + 4, kCursorTmpl, /*attr=*/0u, RQ_OVERLAY);
  emitMenuSprites(160, 146, kPadDiagTmpl, /*attr=*/0u, RQ_OVERLAY);
}
