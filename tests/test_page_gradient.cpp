// The widened-margin rule for an authored full-screen 2D page (game/render/page_gradient.*).
//
// It is tested hermetically because it is pure arithmetic over the canvas width and the authored
// corner colours — the same way the framework tests its own widescreen 2D rule (rq_2d_xform). The
// defect it exists to prevent is issue 0010: at 16:9 the page drew at a 1.335 aspect inside a
// 16:9 target, gaining zero horizontal coverage, and the check that passed it could not tell a
// widened picture from a rescaled one. So the NEGATIVE is asserted first and explicitly: at 4:3 the
// rule must produce nothing at all, or every 4:3 capture and oracle comparison would change.
#include "../game/render/page_gradient.h"

#include <cstdio>

using namespace tomba::render;

namespace {

int gFailures = 0;

void check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAIL: %s\n", what);
    gFailures++;
  }
}

void checkRgb(Rgb got, Rgb want, const char *what) {
  const bool ok = got.r == want.r && got.g == want.g && got.b == want.b;
  if (!ok) {
    std::printf("FAIL: %s — got (%d,%d,%d) want (%d,%d,%d)\n", what, got.r, got.g, got.b, want.r, want.g, want.b);
    gFailures++;
  }
}

// The guest FUN_8007FC24 packet bytes this title reproduces, restated here so the test fails if the
// authored gradient is ever changed by accident rather than agreeing with whatever it now says.
constexpr Rgb kBright{0, 0, 0x46};
constexpr Rgb kDark{0, 0, 0x10};

void authoredGradientIsTheGuestPacket() {
  const PageGradient &page = PageGradient::optionsPage();
  checkRgb(page.topLeft(), kBright, "TL is the bright dark-blue");
  checkRgb(page.topRight(), kBright, "TR is the bright dark-blue");
  checkRgb(page.bottomLeft(), kBright, "BL is the bright dark-blue");
  checkRgb(page.bottomRight(), kDark, "BR is the darker corner");
}

// THE NEGATIVE, FIRST: a 4:3 canvas has no margin, so the rule must emit nothing. A rule that
// returned bands here would put two extra prims into every 4:3 frame and change captures that are
// required to stay byte-identical.
void fourByThreeProducesNoBands() {
  PageMarginBand bands[kMaxPageMarginBands];
  check(pageMarginBands(320, 320, PageGradient::optionsPage(), bands) == 0, "320 into 320 produces no bands");
  check(pageMarginBands(300, 320, PageGradient::optionsPage(), bands) == 0, "a narrower canvas produces no bands");
  check(pageMarginBands(321, 320, PageGradient::optionsPage(), bands) == 0, "a sub-2px canvas gain produces no bands");
}

// The measured shipping geometry: a 320-wide page inside the 428-wide 16:9 render this title
// reports as `render_width=428`.
void widescreenBandsAbutThePage() {
  PageMarginBand bands[kMaxPageMarginBands];
  const int count = pageMarginBands(428, 320, PageGradient::optionsPage(), bands);
  check(count == 2, "a widened canvas produces a left and a right band");
  if (count != 2) {
    return;
  }
  const int margin = (428 - 320) / 2; // 54
  check(bands[0].x0 == 0, "the left band starts at the canvas edge");
  check(bands[0].x1 == margin, "the left band ends where the centred page begins");
  check(bands[1].x0 == margin + 320, "the right band starts where the centred page ends");
  check(bands[1].x1 == 428, "the right band ends at the canvas edge");
  // No seam and no overlap: the two bands plus the centred page cover the canvas exactly once.
  check((bands[0].x1 - bands[0].x0) + 320 + (bands[1].x1 - bands[1].x0) == 428,
        "the bands and the page tile the canvas");
}

// The continuation itself: each band carries the colour the authored gradient already has at the
// edge it touches. Stretching would instead spread the bottom-right darkening across the canvas and
// change the page inside the 4:3 region, which is what issue 0010 rules out.
void bandsContinueTheAuthoredEdges() {
  PageMarginBand bands[kMaxPageMarginBands];
  const int count = pageMarginBands(428, 320, PageGradient::optionsPage(), bands);
  check(count == 2, "bands exist to inspect");
  if (count != 2) {
    return;
  }
  checkRgb(bands[0].color.top, kBright, "the left band's top continues TL");
  checkRgb(bands[0].color.bottom, kBright, "the left band's bottom continues BL");
  checkRgb(bands[1].color.top, kBright, "the right band's top continues TR");
  checkRgb(bands[1].color.bottom, kDark, "the right band's bottom continues BR");
  // The left edge of this particular page is uniform and its right edge is not; the rule must carry
  // that difference rather than flattening both to one colour.
  check(bands[1].color.top.b != bands[1].color.bottom.b, "the right band keeps the page's vertical fall");
}

// A canvas that is wider still keeps the page the same size and only grows the margins: the page is
// never scaled to fit the canvas.
void widerCanvasGrowsOnlyTheMargins() {
  PageMarginBand narrow[kMaxPageMarginBands];
  PageMarginBand wide[kMaxPageMarginBands];
  check(pageMarginBands(428, 320, PageGradient::optionsPage(), narrow) == 2, "428 produces bands");
  check(pageMarginBands(640, 320, PageGradient::optionsPage(), wide) == 2, "640 produces bands");
  check(wide[0].x1 > narrow[0].x1, "a wider canvas has a wider left margin");
  check((wide[1].x0 - wide[0].x1) == (narrow[1].x0 - narrow[0].x1), "the page keeps its authored width");
}

// A defensive contract rather than a crash: no output buffer, or a page with no authored width, is a
// caller error that must produce nothing instead of writing through a null pointer.
void refusesImpossibleInputs() {
  PageMarginBand bands[kMaxPageMarginBands];
  check(pageMarginBands(428, 320, PageGradient::optionsPage(), nullptr) == 0, "no buffer produces no bands");
  check(pageMarginBands(428, 0, PageGradient::optionsPage(), bands) == 0, "a zero-width page produces no bands");
  check(pageMarginBands(428, -320, PageGradient::optionsPage(), bands) == 0, "a negative page width produces no bands");
}

} // namespace

int main() {
  authoredGradientIsTheGuestPacket();
  fourByThreeProducesNoBands();
  widescreenBandsAbutThePage();
  bandsContinueTheAuthoredEdges();
  widerCanvasGrowsOnlyTheMargins();
  refusesImpossibleInputs();
  if (gFailures != 0) {
    std::printf("page_gradient: %d check(s) FAILED\n", gFailures);
    return 1;
  }
  std::printf("page_gradient: all checks passed\n");
  return 0;
}
