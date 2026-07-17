// NATIVE producer for the DEMO/title front-end LOAD-GAME memory-card BROWSER (renderTitle sm[0x48]==4).
//
// Reached: title page 0 -> cursor on "Load Game" -> Confirm. Demo::s2 sets sm[0x48]=4 and hands the card
// flow to the substrate state machine 0x8007BF20 (phase byte @0x800BF84A), whose UI driver 0x8007BE18
// dispatches the per-screen renderers (the confirmed slot-select screen is 0x8007F250). Under pc_render
// (PSXPORT_GATE=1) s4 EXECUTES on the substrate; this producer supplies the PICTURE at render time.
//
// WHAT THE CARD SCREEN DRAWS (RE'd from MAIN.EXE via Ghidra headless, scratch/decomp/card.c):
//   FUN_8007f250 (slot-select) is entirely: text emitters (drawText/drawTextSmall/icon), the CURSOR
//   sprite via FUN_8007e998, and the full-screen BACKDROP via FUN_8007fc24. There are NO FT4 slot boxes
//   on this screen (the "slots" are text rows the cursor points at).
//   · TEXT — already emitted natively: Font::glyphEmit/drawText/drawTextSmall/iconGlyphTap are globally
//     tapped (font_wide_re_install, game_tomba2.cpp), so every card-screen glyph is pushed to RQ_HUD
//     during s4 execution. This producer must NOT re-draw text.
//   · BACKDROP — FUN_8007fc24: one OPAQUE full-screen POLY_G4 gouraud quad, a dark-blue vertical
//     gradient (TL/TR/BL blue B=0x46, BR darker B=0x10). Reproduced here as an RQ_BACKGROUND quad.
//   · CURSOR — FUN_8007e998(x=44, y): the slot-select cursor sprite. FUN_8007e998 always draws menu
//     template 0x98 (the same resident cursor template the title menu uses — menuItemsAndCursor), at
//     y = slotY[sel]+4 where slotY = {96,128} and sel = mem_r8(0x800BF808) (the highlight/selection
//     index). Reproduced via the proven Render::emitMenuFt4 decoder (reads the game's own menu template
//     table @0x80017334 + data @0x80158000 — NOT a GP0 packet), attr 0 (raw/bright), RQ_OVERLAY.
//
// READ-ONLY overlay: reads guest RAM (selection byte 0x800BF808) + the menu templates emitMenuFt4 reads;
// writes only the host render queue. No GTE/OT/GP0/depth — explicit 2D layering (BACKGROUND<OVERLAY<HUD).
//
// SCOPE / LIMITATION (honest): the slot-select screen (0x8007F250) is fully reproduced. The transient
// sibling card screens ("Checking MEMORY CARD...", "No data for Tomba!2", "Select file to be loaded")
// also live under s48==4 and are drawn by the runtime LOAD-MENU overlay (0x8018a000, FUN_8018fa88/fbcc,
// not statically RE-able here). Their TEXT auto-arrives via the glyph tap and they share the same
// dark-blue backdrop, so this producer draws them correctly EXCEPT that the slot cursor is emitted on
// every phase. On a phase with no slot cursor (e.g. the "No data" message) that shows a stray cursor at
// the slot-1 position; if the operator sees that on-screen, gate the cursor emission on the browser
// slot-select sub-state (dump the 0x8018a000 overlay live and RE FUN_8018fa88/fbcc to pin the flag).
#include "core.h"
#include "game.h"
#include "render.h"
#include "render_queue.h"

// renderCardBrowser — see render.h. Read-only native producer for the DEMO/title Load-Game memory-card
// browser (sm[0x48]==4). Backdrop gradient + slot cursor; the card-screen TEXT arrives from the global
// font glyph tap during s4 execution. Called from Render::renderTitle under its DisplayPassGuard.
void Render::renderCardBrowser() { Core* c = mCore;
  // WIDESCREEN PILLARBOX: a flat-black full-screen fill (all vertex colours equal → it STRETCHES to the
  // wide FB, painting the side margins black) behind the 4:3 gradient below (which is non-flat → it
  // CENTERS instead of stretching). Without this the pillarbox bars would show stale VRAM. 4:3: no-op.
  { int xs[4] = { 0, 320, 0, 320 }, ys[4] = { 0, 0, 240, 240 }, z[4] = { 0, 0, 0, 0 };
    unsigned char k[4] = { 0, 0, 0, 0 };
    c->game->activeRq().push2dQuad(RQ_BACKGROUND, /*order_2d_fg=*/0, xs, ys, z, z, k, k, k,
                                   0, 0, /*mode=*/3, /*raw=*/0, 0, 0, 0, 0, 0, 0, 0, 0, 1023, 511); }
  // BACKDROP — reproduce FUN_8007fc24: opaque full-screen dark-blue vertical gradient (per-vertex color).
  // Screen-space fill (no display offset, exactly like Render::menuChrome's backdrop quad).
  {
    int xs[4] = { 0, 320, 0, 320 };
    int ys[4] = { 0,   0, 240, 240 };
    int uv[4] = { 0,   0,   0,   0 };
    unsigned char rz[4] = { 0, 0, 0, 0 };                 // R = 0 on every vertex
    unsigned char gz[4] = { 0, 0, 0, 0 };                 // G = 0 on every vertex
    unsigned char bs[4] = { 0x46, 0x46, 0x46, 0x10 };     // B: TL,TR,BL bright, BR darker (the gradient)
    c->game->activeRq().push2dQuad(RQ_BACKGROUND, /*order_2d_fg=*/0, xs, ys, uv, uv, rz, gz, bs,
                                   /*tp_x=*/0, /*tp_y=*/0, /*mode=*/3, /*raw=*/0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 1023, 511);
  }
  // CURSOR — reproduce FUN_8007e998(44, slotY[sel]+4) with the resident cursor template 0x98 (raw/bright).
  static const int SLOTY[2] = { 96, 128 };                // FUN_8007f250 auStack_20 = {0x60, 0x80}
  const int sel = c->mem_r8(0x800BF808u) & 1;             // highlight/selection index (0 or 1)
  emitMenuFt4(/*anchorX=*/44, /*anchorY=*/SLOTY[sel] + 4, /*templateIdx=*/0x98u, /*attr=*/0u, RQ_OVERLAY);
}
