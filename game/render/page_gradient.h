// The authored full-screen 2D page background of guest FUN_8007FC24, and the rule for what the
// widened canvas shows BESIDE it.
//
// ONE DEFINITION OF THE GRADIENT. FUN_8007FC24 draws a single untextured POLY_G4 covering
// (0,0)-(320,240) whose per-vertex blue is TL/TR/BL = 0x46 and BR = 0x10 — a subtle darkening toward
// the bottom-right. Two native producers reproduce that same guest function: Render::optionsBackdrop
// (the five-page Options family, title and in-game) and Render::renderCardBrowser (the memory-card
// page). Each had typed the corner colours out for itself, so one authored gradient existed as two
// literals that nothing kept in step. It lives here once, and both call it.
//
// WHY THE MARGIN NEEDS A RULE AT ALL. Every full-screen page in this title is authored 320 wide, so
// at 16:9 the page covers the middle 320 of a 428-wide render and something must occupy the two
// margins. Measured 2026-09-19 (issue 0010), the page drew at a 1.335 aspect inside a 16:9 target:
// the same 320-wide page with margins either side, gaining zero horizontal coverage.
//
// WHAT THIS IS NOT. The margin is NOT produced by stretching: spreading the gradient across 428
// columns would move the authored bottom-right darkening to a different screen position and change
// the page inside the 4:3 region, which is the one thing issue 0010 rules out. It is not produced by
// sampling the frame, inspecting neighbouring pixels, or choosing content from the picture either.
//
// WHAT IT IS. A CLAMP-CONTINUATION of the page's own edge: each margin band carries the colour the
// authored gradient already has at the edge it touches, held constant outward. The left edge is
// (TL, BL) and the right edge is (TR, BR), so the left band is flat 0x46 and the right band carries
// the same 0x46 -> 0x10 vertical fall the page's right edge has. Every pixel inside the authored 320
// is untouched, the seam is exactly the colour on both sides of it, and nothing is invented: the
// value at margin x is the value the authored gradient defines at the boundary.
//
// THE RULE IS PURE ARITHMETIC, deliberately — no Core, no RenderQueue, no globals — so it is tested
// hermetically the way the framework tests its own widescreen 2D rule (rq_2d_xform). PageBackdrop
// applies it; this decides it.
#pragma once

namespace tomba::render {

// One vertex colour of an authored page. PSX colours are 8-bit per channel.
struct Rgb {
  unsigned char r = 0;
  unsigned char g = 0;
  unsigned char b = 0;
};

// One vertical edge of an authored page: the colours of its top and bottom vertices.
struct PageEdge {
  Rgb top;
  Rgb bottom;
};

// A margin band of the widened canvas, in WIDE-FINAL x (the canvas's own coordinates, not a 4:3
// author's), carrying the clamp-continued colour of the page edge it touches.
struct PageMarginBand {
  int x0 = 0;
  int x1 = 0;
  PageEdge color;
};

// The authored screen extent every full-screen page in this title fills, and the extent the guest's
// own full-screen quads use (render_options.cpp's 0..320 x 0..240 gradient, pause_menu.cpp's 320x240
// tile). These are AUTHORED 4:3 coordinates; the widened canvas width is a runtime value.
inline constexpr int kPageWidth = 320;
inline constexpr int kPageHeight = 240;

// At most a left and a right band; a widened canvas never needs more.
inline constexpr int kMaxPageMarginBands = 2;

// The authored full-screen page gradient: four corner colours, in TL, TR, BL, BR order — the order
// push2dQuad takes its vertex arrays in.
class PageGradient {
public:
  constexpr PageGradient(Rgb topLeft, Rgb topRight, Rgb bottomLeft, Rgb bottomRight)
      : mTopLeft(topLeft), mTopRight(topRight), mBottomLeft(bottomLeft), mBottomRight(bottomRight) {}

  // THE definition of guest FUN_8007FC24's dark-blue page gradient, shared by every producer that
  // reproduces it: the Options family (Render::optionsBackdrop) and the memory-card browser
  // (Render::renderCardBrowser).
  static const PageGradient &optionsPage();

  // THE definition of the pause/item menu's own full-screen backdrop: the guest's GP0 0x60 tile at
  // (0,0) 320x240, colour 0x000000. Uniform black, so its clamp-continuation is black margins —
  // which is a CONSEQUENCE of what that page is authored as, not a fill chosen for every page. The
  // guest builds this quad itself (PauseMenu's pushScreenQuad); only the margins come from here,
  // because widening the guest quad would make a faithful producer draw geometry the guest never
  // submitted.
  static const PageGradient &pauseMenu();

  constexpr Rgb topLeft() const {
    return mTopLeft;
  }
  constexpr Rgb topRight() const {
    return mTopRight;
  }
  constexpr Rgb bottomLeft() const {
    return mBottomLeft;
  }
  constexpr Rgb bottomRight() const {
    return mBottomRight;
  }

  // The colours the gradient already has at each vertical edge — what a margin band continues.
  constexpr PageEdge leftEdge() const {
    return PageEdge{mTopLeft, mBottomLeft};
  }
  constexpr PageEdge rightEdge() const {
    return PageEdge{mTopRight, mBottomRight};
  }

private:
  Rgb mTopLeft;
  Rgb mTopRight;
  Rgb mBottomLeft;
  Rgb mBottomRight;
};

// The margin bands that clamp-continue `page` across a canvas `canvasWidth` wide around an authored
// page `nativeWidth` wide, centred the way RenderQueue centres an authored 4:3 submission. Writes up
// to kMaxPageMarginBands entries and returns how many it wrote: 0 when the canvas is not wider than
// the page, which is the 4:3 case and must emit nothing at all.
int pageMarginBands(int canvasWidth, int nativeWidth, const PageGradient &page, PageMarginBand *bands);

} // namespace tomba::render
