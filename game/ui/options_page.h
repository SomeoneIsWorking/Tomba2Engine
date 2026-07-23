// class OptionsPage — native display producer for the OPTIONS page family (kanban #38, extending #7).
//
// THE PAGE. Five builders, selected by the controller FUN_8007B45C through its bounds-checked jump
// table at 0x80016E78 on task-sm[0x50]:
//     0 -> FUN_8007F104  "Select Options"   1 -> FUN_8007F250  "Messages"
//     2 -> FUN_8007F498  "Sound"            3 -> FUN_8007F73C  "Screen adjust"
//     4 -> FUN_8007F8F8  "Controls"
// The SAME five builders serve two entry points: the DEMO/title front-end (stage 0x801062E4,
// sm[0x48]==6) and the IN-GAME page dispatcher FUN_8010810C with page byte task-sm[0x6B]==3 (raised
// from the START page's "Options" item). One page, one producer — this class — for both.
//
// WHAT WAS MISSING (kanban #38, measured 2026-07-23 on the in-game leg, replay
// replays/bugs/ingame-options-page.pad at frame 1160, 4:3 / fps60 off):
//   * psx_render: the whole screen is the page. `debug otattr` that frame lists 83 ordering-table
//     packets and NOT ONE of them is world geometry — packet [2047] is `op=0x38` (POLY_G4) at
//     v0=(0,0) attributed to fn=0x8010810C, i.e. the full-screen dark-blue gradient FUN_8007FC24,
//     followed by one `op=0x2D` FT4 (the feather cursor, fn=0x8007E2F8 inside the shared group leaf
//     FUN_8007E1B8) and 65 `op=0x65` glyph sprites.
//   * pc_render: the live field showed straight through the page — 74442/76800 pixels (96.9%)
//     different from the psx leg at the same frame. The backdrop had NO native producer on this path
//     at all: Render::optionsBackdrop existed as a host twin but was only ever reached from
//     Render::renderTitle's front-end branch, so nothing invoked it in-game.
//
// THE PRODUCER = THE EMITTERS, SCOPED BY THE PAGE. Rather than a second host reproduction driven off
// the in-game dispatcher, each element is produced where the guest emits it, and the page scope puts
// those pieces in the guest's paint order:
//   * BACKDROP  FUN_8007FC24 — owned here (pushBackdrop below is a real port of the emitter, not a
//     tap: 36-byte POLY_G4 packet, guest-exact) and its picture is drawn by Render::optionsBackdrop.
//   * BOXES     FUN_8007FCC8 — already ported as Panel::pushDialogBackdrop (game/ui/dialog_backdrop.cpp);
//     that ONE owner records the rectangle here when this scope is raised, so the dialog path (its own
//     producer) is untouched. Page 3 only.
//   * CURSOR / PAD DIAGRAM — the shared 2D group leaves FUN_8007E1B8 / FUN_8007E6DC, whose single
//     owners (game/render/ui_ft4_tap.cpp, game/ui/ui_sprite.cpp) hand every group to
//     UiGroupCapture::route, which files it under whichever page scope is raised.
//   * TEXT — the global Font/icon taps (game/ui/font.cpp) at RQ_HUD. Producers must never re-draw it.
// So there is exactly ONE install per guest address and no host twin of a page's element list.
//
// PAINT ORDER AND LAYER. The guest calls the cursor BEFORE the backdrop (gen_func_8007F104:
// func_8007E998 then func_8007FC24) but links the backdrop DEEPER in the ordering table, so call order
// is not paint order — the backdrop goes down first, then the boxes newest-first (a bucket's list is
// LIFO), then the captured groups in UiGroupCapture::paintOrder. Everything lands at RQ_OVERLAY, one
// band BELOW the glyph text's RQ_HUD (a page fill at RQ_HUD paints over its own text — bug #64, then
// kanban #28), and in the 2D-FG depth band so the backdrop covers the live field the in-game page is
// raised over (the 2D-BG band sits behind the 3D world by construction).
//
// Read-only overlay: the guest half is a byte-exact port / the untouched gen bodies; the display half
// writes host memory only and is skipped on the oracle and psx_render legs.
#pragma once
#include <cstdint>
#include <vector>
#include "ui_group_capture.h"
class Core;

class OptionsPage {
public:
  // The page's chrome groups (cursor, pad diagram), captured off the shared 2D group leaves for as
  // long as one of the five page builders is on the stack. Per-Core, so SBS's two cores cannot see
  // each other's scope.
  UiGroupCapture capture;

  // FUN_8007FC24 fired inside the scope this frame (pages 0/1/2/4; page 3 draws no backdrop).
  bool mBackdrop = false;

  // FUN_8007FCC8 rectangles staged inside the scope, in CALL order (page 3's header/row/footer boxes).
  struct Box { int x, y, w, h; uint32_t flags; };
  std::vector<Box> mBoxes;

  // install(): the five page-builder scope wrappers + the backdrop emitter FUN_8007FC24. Idempotent;
  // called from games_tomba2_init alongside the other *_install() wirings.
  static void install();

  // pushBackdrop: the PORT of FUN_8007FC24 — one 36-byte POLY_G4 packet (the 320x240 dark-blue
  // gradient) allocated off the guest packet pool and linked at ordering-table bucket 1. Guest half
  // only; the picture is Render::optionsBackdrop, deferred to drawCollected for paint order.
  static void pushBackdrop(Core* c);

  // noteBox: called by the FUN_8007FCC8 owner (Panel::pushDialogBackdrop's guest-ABI entry) after the
  // guest packet is staged. A no-op unless this page's scope is raised, so the dialog box is untouched.
  static void noteBox(Core* c, int x, int y, int w, int h, uint32_t flags);

  // drawCollected: push the frame's backdrop, boxes and captured groups to the render queue in the
  // guest's paint order at RQ_OVERLAY. Called once per page-builder run.
  void drawCollected(Core* c);
};
