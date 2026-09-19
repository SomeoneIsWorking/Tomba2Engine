// The authored page gradient and the margin rule. See page_gradient.h for the measurement that
// identified the uncovered margin and for why continuation, not stretching, is the answer.
#include "page_gradient.h"

namespace tomba::render {

// Guest FUN_8007FC24's packet bytes, verbatim: red and green are zero on every vertex and the blue
// channel carries the whole gradient — TL/TR/BL bright (0x46), BR darker (0x10).
const PageGradient &PageGradient::optionsPage() {
  static constexpr Rgb kBright{0, 0, 0x46};
  static constexpr Rgb kDark{0, 0, 0x10};
  static constexpr PageGradient kPage{kBright, kBright, kBright, kDark};
  return kPage;
}

// The pause/item menu's guest tile: opaque black over the whole authored screen, read back with the
// REPL `rw` on the psx_render leg 2026-07-22 (pool 0x800C20B0 = `60000000 00000000 00F00140`).
const PageGradient &PageGradient::pauseMenu() {
  static constexpr Rgb kBlack{0, 0, 0};
  static constexpr PageGradient kPage{kBlack, kBlack, kBlack, kBlack};
  return kPage;
}

int pageMarginBands(int canvasWidth, int nativeWidth, const PageGradient &page, PageMarginBand *bands) {
  if (bands == nullptr || nativeWidth <= 0) {
    return 0;
  }
  // The same centring RenderQueue applies to an authored 4:3 submission (rq_2d_xform's `margin`), so
  // the bands abut the page exactly rather than leaving or overlapping a seam column.
  const int margin = (canvasWidth - nativeWidth) / 2;
  if (margin <= 0) {
    return 0; // 4:3, or a canvas that is not actually wider: the page is already the whole picture
  }
  bands[0] = PageMarginBand{0, margin, page.leftEdge()};
  bands[1] = PageMarginBand{margin + nativeWidth, canvasWidth, page.rightEdge()};
  return 2;
}

} // namespace tomba::render
