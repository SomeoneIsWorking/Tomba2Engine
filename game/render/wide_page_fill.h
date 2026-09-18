// class WidePageFill — the ONE owner of "an authored 4:3 full-screen 2D page must not leave the
// widened canvas showing through beside it".
//
// THE DEFECT IT FIXES, measured 2026-09-19 at 16:9 on the shipping product. The pause/item menu
// (replays/bugs/ingame-item-menu.pad f1120) and the in-game Select Options page
// (replays/bugs/ingame-options-page.pad f1160) are both OPAQUE FULL-SCREEN pages: at 4:3 their art
// reaches all four screen edges and the field behind them is completely hidden. Both are authored
// 320 wide, so in widescreen they keep a 4:3 extent and the live field reappears in the side
// margins, cut off by a hard vertical edge at each end of the page.
//
// WHY THE EXISTING ATTEMPTS DID NOT COVER IT. Two callers already pushed a private "pillarbox" quad
// — the same 22-argument block, copied — and each carried the same comment claiming that a flat
// untextured quad STRETCHES across the wide framebuffer. That is only half the framework's rule:
// rq_2d_xform (external/psxport/runtime/psx/render_queue.cpp) spreads a flat untextured fill only on
// RQ_BACKGROUND. Render::optionsBackdrop pushes its copy on RQ_OVERLAY — it must, because the
// in-game page is raised over a LIVE field frame and the 2D-BG band sits behind the 3D world — so
// that copy was centred like any other 4:3 quad and painted nothing outside the page. The producer
// census recorded a `pc/options-pillarbox` row for a quad that could never reach a margin.
//
// WHAT THIS OWNER DOES INSTEAD OF RELYING ON THAT RULE. It states the geometry rather than hoping a
// material heuristic infers it: one flat black quad spanning the WHOLE canvas, declared
// RQ_2D_WIDE_FINAL so the queue's 4:3 centring leaves it alone, on the page's own layer and band.
// It is deterministic — a projection/presentation extent, no frame sampling, no content-dependent
// choice of what appears in the added area — and it reads no guest memory beyond the canvas width.
//
// AT 4:3 IT EMITS NOTHING. There is no margin to cover, so the page's own art is already the whole
// picture and an extra prim would only be a prim. That also keeps every existing 4:3 capture and
// oracle comparison byte-identical.
//
// WHERE IT DOES NOT APPLY. A page that composites over the live field ON PURPOSE — the in-game START
// page (StartPage), the dialog box, the field HUD — is not full-screen and must keep showing the
// world. Only a caller that knows its page is opaque and full-screen calls this.
#pragma once

class Core;
struct RenderQueue;

namespace tomba::render {

class WidePageFill {
public:
  // The presentation canvas width in wide-final coordinates: the widened framebuffer width under
  // widescreen, the game's own 4:3 width otherwise.
  static int canvasWidth(Core &core);

  // True when the canvas is wider than the game's authored 4:3 width, i.e. there is a margin for an
  // authored full-screen page to leave uncovered.
  static bool widened(Core &core);

  // Push the canvas-wide black fill for one opaque full-screen 2D page, before that page's own art.
  // `layer` and `order2dFg` are the page's own band, so the fill lands directly behind it. No-op at
  // 4:3.
  static void pushBehindPage(Core &core, RenderQueue &queue, int layer, int order2dFg);
};

} // namespace tomba::render
