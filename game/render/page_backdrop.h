// class PageBackdrop — the ONE owner of an authored full-screen 2D page's background: the authored
// gradient itself, and what the widened canvas shows beside it.
//
// WHY ONE OWNER. Two native producers reproduce the SAME guest background function FUN_8007FC24 —
// Render::optionsBackdrop (the five-page Options family, title and in-game) and
// Render::renderCardBrowser (the memory-card page). Each had typed out its own copy of the same
// 22-argument quad push AND its own copy of the corner colours, so one authored gradient existed as
// two literals that nothing kept in step. Both now call this.
//
// THE WIDESCREEN DEFECT IT FIXES, measured 2026-09-19 at 16:9 on the shipping product. Every
// full-screen page in this title is authored 320 wide, so at 16:9 it covers the middle 320 of a
// 428-wide render and something must occupy the two margins. Originally the live field showed
// through them behind a hard vertical edge (pause/item menu f1120, in-game Options page f1160). The
// first fix covered the margin with one canvas-wide BLACK quad, which closed that hole and opened
// another: issue 0010 measured the page drawing at a 1.335 aspect inside a 16:9 target — the same
// 320-wide page with black pillars either side, gaining zero horizontal coverage.
//
// WHAT THE MARGIN CARRIES NOW. The page's own edge colour, continued outward. page_gradient.h owns
// the authored gradient and decides the bands; this class draws them. The left band is flat 0x46
// blue and the right band carries the same 0x46 -> 0x10 vertical fall the page's right edge has, so
// the page reads as one surface across the whole canvas.
//
// NOT BY STRETCHING. Spreading the gradient across 428 columns would move the authored bottom-right
// darkening to a different screen position and change the page inside the 4:3 region, which is the
// one thing issue 0010 rules out. The margin is a clamp-continuation: deterministic, computed from
// the canvas width and the authored corner colours, with no frame sampling and no content-dependent
// choice of what appears in the added area. It reads no guest memory at all.
//
// AT 4:3 THE MARGINS EMIT NOTHING. There is no margin to cover, so the page's own art is already the
// whole picture and an extra prim would only be a prim. That keeps every existing 4:3 capture and
// oracle comparison byte-identical.
//
// WHERE THE MARGINS DO NOT APPLY. A page that composites over the live field ON PURPOSE — the
// in-game START page (StartPage), the dialog box, the field HUD — is not full-screen and must keep
// showing the world. Only a caller that knows its page is opaque and full-screen asks for them.
#pragma once

class Core;
struct RenderQueue;

namespace tomba::render {

class PageGradient;

class PageBackdrop {
public:
  // The presentation canvas width in wide-final coordinates: the widened framebuffer width under
  // widescreen, the game's own 4:3 width otherwise.
  static int canvasWidth(Core &core);

  // True when the canvas is wider than the game's authored 4:3 width, i.e. there is a margin for an
  // authored full-screen page to leave uncovered.
  static bool widened(Core &core);

  // The authored page gradient itself, over its authored 320x240 extent, on the page's own band.
  // Identical at 4:3 and 16:9 — widescreen changes what is BESIDE the page, never the page.
  static void pushAuthored(Core &core, RenderQueue &queue, int layer, int order2dFg, const PageGradient &page);

  // Continue `page`'s own edge colours into the widened canvas margins, on the page's own band, so
  // they land directly behind the page's authored art. No-op at 4:3.
  static void pushMargins(Core &core, RenderQueue &queue, int layer, int order2dFg, const PageGradient &page);
};

} // namespace tomba::render
