// NATIVE pc_render producer for the DEMO/title front-end OPTIONS page (stage 0x801062E4, sm[0x48]==6).
//
// Reached from Render::renderTitle's s48==6 branch. This is the DEMO front-end OPTIONS front page: the
// substate the title's "Options" item confirms into. The substrate front-end task runs the real page
// controller FUN_8007B45C -> FUN_8007F104 UNDERNEATH this display pass every frame (page selected by
// task+0x50; Demo::s3 seeds it 0 before entering s6, so DEMO s6 is always page0). We reproduce
// FUN_8007F104's OBSERVABLE picture from guest state, host-only, into the render queue.
//
// WHAT FUN_8007F104 draws (RE'd from gen_func_8007F104, generated/shard_4.c:12354):
//   1. header icon-string  FUN_80078988(38,200, str=0x800172AC)      -> font/icon TAP (iconGlyphTap)
//   2. title label         FUN_80079324(50,200,6, str=*(cfg+160))    -> font TAP (drawTextSmall dual-emit)
//   3. static header       FUN_80079374(32,32,  str=*(cfg+48))       -> font TAP (drawText dual-emit)
//   4. 4 option labels      FUN_80079374 loop, y=76+24*i,
//        str=*(cfg+52+4*i), size 8 if i==cursor else 0 (clut = bright/dim) -> font TAP (drawText dual-emit)
//   5. cursor sprite        FUN_8007E998(cursorX, 80+24*cursor)       -> THIS producer (FT4 template 0x98)
//   6. background gradient  FUN_8007FC24 (one full-screen POLY_G4)    -> THIS producer (RQ_BACKGROUND)
//
// The TEXT (items 1-4) is emitted to the render queue by the existing font dual-emit TAPS
// (Font::drawText/drawTextSmall/iconGlyphTap glyphQueuePush, game/ui/font.cpp) that fire while the
// substrate executes FUN_8007F104 on the faithful task path — exactly like the field HUD/dialog text.
// So this producer only needs to add the two NON-font, guest-packet elements the taps don't cover:
// the CURSOR sprite (an FT4 menu template, same 0x98 template as the title cursor) and the BACKGROUND
// gradient quad. READ-ONLY: reads guest RAM only, emits host-only quads (wrapped in DisplayPassGuard by
// the caller). No gte_op / OT / GP0 packet reading. See docs/findings/ui.md, render_walk.cpp emitMenuFt4.
#include "core.h"
#include "game_ctx.h"
#include "render.h"
#include "game.h"
#include "render_queue.h"
#include <stdint.h>

// OPTIONS config/string table base (gen: r16 = 32778<<16 + 10324). The 4 option strings live at
// cfg+52/+56/+60/+64; FUN_800793C4 centers each and derives the cursor X anchor from them.
static constexpr uint32_t kOptCfgBase   = 0x800A2854u;
static constexpr uint32_t kMenuCursorB  = 0x800BF808u;   // shared menu cursor byte (u8 selected item)
static constexpr uint32_t kTaskSmPtr    = 0x1F800138u;   // scratchpad *-> current task state machine
static constexpr uint32_t kCursorTmpl   = 0x98u;         // FT4 cursor template index (== title cursor)

// FUN_8009A600 — plain strlen over a guest C-string (counts bytes to the NUL). Read-only.
static int optStrLen(Core* c, uint32_t s) {
  int n = 0;
  while (s && c->mem_r8(s + (uint32_t)n) != 0) n++;
  return n;
}

// cursorAnchorX — reproduce FUN_800793C4's return value (the cursor X anchor). FUN_800793C4 measures
// each of the 4 option strings, lays each out at X[i] = 160 - (strlen(str[i])>>1)*8 (centered around
// x=160, 8px/char), tracks the MINIMUM X (leftmost item), and returns that minimum - 16 (the cursor
// sits 16px left of the leftmost option). r19 starts at 160, so an empty/absent string can't push the
// min above 160. Equivalent closed form: 160 - 8*max_i(strlen(str[i])>>1) - 16.
static int cursorAnchorX(Core* c) {
  int maxHalf = 0;
  for (int i = 0; i < 4; i++) {
    const uint32_t str = c->mem_r32(kOptCfgBase + 52u + (uint32_t)i * 4u);
    const int half = optStrLen(c, str) >> 1;
    if (half > maxHalf) maxHalf = half;
  }
  return 160 - maxHalf * 8 - 16;
}

// optionsPageNative — the DEMO front-end OPTIONS page-0 producer. See file banner.
void Render::optionsPageNative() {
  Core* c = mCore;

  // PAGE GATE: FUN_8007B45C dispatches 5 option sub-pages on *(u16)(*0x1F800138 + 0x50). DEMO s6 only
  // ever enters page0 (Demo::s3 seeds sm[0x50]=0); pages 1-4 (Messages/Sound/Screen/Controls) have
  // their own layouts (FUN_8007F250/F498/F73C/F8F8) and are not built yet — fail-fast so they surface
  // as the next rebuild item rather than drawing a wrong page. sm==0 (no current task) defaults to page0.
  const uint32_t sm = c->mem_r32(kTaskSmPtr);
  const int page = sm ? (int)c->mem_r16(sm + 0x50u) : 0;
  if (page != 0)
    abortUnimplemented("DEMO OPTIONS sub-page (sm[0x50] != 0) — only page0 has a native producer");

  // WIDESCREEN PILLARBOX: flat-black full-screen fill (equal vertex colours → STRETCHES to fill the wide
  // FB, blacking the side margins) behind the 4:3 gradient below (non-flat → CENTERS, not stretched). 4:3: no-op.
  { int xs[4] = { 0, 320, 0, 320 }, ys[4] = { 0, 0, 240, 240 }, z[4] = { 0, 0, 0, 0 };
    unsigned char k[4] = { 0, 0, 0, 0 };
    c->game->activeRq().push2dQuad(RQ_BACKGROUND, /*order_2d_fg=*/0, xs, ys, z, z, k, k, k,
                                   0, 0, /*mode=*/3, /*raw=*/0, 0, 0, 0, 0, 0, 0, 0, 0, 1023, 511); }
  // (d) BACKGROUND — reproduce FUN_8007FC24: ONE full-screen POLY_G4 (op 0x38) Gouraud gradient quad
  // covering (0,0)-(320,240). Vertex colors (packet bytes, verbatim): TL/TR/BL = RGB(0,0,70) dark blue,
  // BR = RGB(0,0,16) — a subtle top->bottom-right darkening. Untextured (mode 3, raw 0), RQ_BACKGROUND
  // so it sits behind the option text (RQ_HUD) and cursor (RQ_OVERLAY). Absolute screen coords, no HUD
  // offset (matches the guest's fixed 0..320 / 0..240 fill).
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

  // (c) CURSOR — reproduce FUN_8007E998(x = FUN_800793C4 return, y = Y[cursor]+4, attr=0), which draws
  // FT4 template 0x98 (the SAME cursor sprite the title uses) via FUN_8007E8DC/FUN_8007E1B8. Option i
  // sits at screen Y = 76 + 24*i; the cursor anchors 4px lower on the selected row. emitMenuFt4 with
  // attr=0 -> raw (bright), exactly as menuItemsAndCursor draws the title cursor.
  int cursor = (int)c->mem_r8(kMenuCursorB);
  if (cursor < 0 || cursor > 3) cursor = 0;              // host-only clamp (valid range is 0..3)
  const int cursorX = cursorAnchorX(c);
  const int cursorY = 76 + 24 * cursor + 4;
  emitMenuFt4(cursorX, cursorY, kCursorTmpl, /*attr=*/0u, RQ_OVERLAY);

  // (a)+(b) title label, static header, and the 4 option labels are emitted to the queue by the font
  // dual-emit taps (Font::drawTextSmall/drawText/iconGlyphTap glyphQueuePush) while the substrate runs
  // FUN_8007F104 underneath — no reproduction needed here (a Font:: call would WRITE guest memory and
  // break the read-only display-pass invariant). The bright/dim selection is already in the tapped
  // glyphs' clut (the guest passes size 8 for the selected item / 0 for the rest, which selects the
  // bright vs dim glyph palette that glyphQueuePush reads back from the scratch struct).
}
