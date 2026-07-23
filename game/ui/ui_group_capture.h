// class UiGroupCapture — the SHARED capture used by every native producer whose page paints its
// chrome through the game-wide 2D group emitters FUN_8007E1B8 (POLY_FT4) and FUN_8007E6DC (SPRT).
//
// WHY IT EXISTS. Those two leaves are game-wide: the pause/item menu (kanban #21), the in-game START
// page (kanban #35) and the field HUD all reach them. Each leaf is owned ONCE — a second
// overrides::install on one address is the dual-ownership bug that broke the dialog box (kanban #28)
// — so the taps live where the address is owned (pause_menu.cpp for 0x8007E1B8, ui_sprite.cpp's
// ov_compose for 0x8007E6DC) and simply hand the arguments to `route`. `route` gives them to
// whichever PAGE SCOPE is currently raised, i.e. to the producer whose controller is on the stack.
// A group emitted with no scope raised has its own producer already (Render::fieldHudRender calls
// emitUiFt4 directly) and is dropped here.
//
// Adding a page is ONE line in `route` (the pause menu #21, the START page #35 and the OPTIONS page
// family #38 are all listed there) — never a second override on a shared leaf.
//
// PAINT ORDER COMES FROM THE GUEST'S ORDERING TABLE, NOT FROM CALL ORDER. `paintOrder` sorts by
// DESCENDING OT bucket (attrs+1 — a lower PSX OT index is walked later, i.e. drawn in front) and,
// within one bucket, in REVERSE call order (AddPrim prepends, so a bucket's list is LIFO). Emitting
// in plain call order is wrong and visibly so — measured on the pause menu, where the black panel
// interior painted over the item icons.
//
// Read-only overlay: host memory only, not one guest write. Per-Core (each producer owns its own
// instance on Engine) so SBS's two cores cannot see each other's scope.
#pragma once
#include <cstdint>
#include <vector>
class Core;

// The shared argument shape lives in game/render/ui_group_args.h — ONE definition. The score-popup
// producer (#18) and the page-chrome capture here both decode the same two leaves, and each
// originally declared its own identical `struct UiGroupArgs`; two definitions of one struct is a
// redefinition error the moment both land, and the near-miss version of it is two decoders drifting
// apart silently. Same lesson as the duplicate-owned guest address (kanban #28), one level up.
#include "render/ui_group_args.h"

class UiGroupCapture {
public:
  // Raised for the duration of one page controller (see the scope wrappers in pause_menu.cpp /
  // start_page.cpp). Only while it is up does `route` record anything into this capture.
  bool mCapturing = false;
  // Groups collected during one controller run, in CALL order. Re-ordered by paintOrder, drained by
  // the owning producer's drawCollected.
  std::vector<UiGroupArgs> mGroups;

  bool capturing() const { return mCapturing; }
  void begin() { mCapturing = true; }
  void end()   { mCapturing = false; }
  void clear() { mGroups.clear(); }
  bool empty() const { return mGroups.empty(); }

  // Decoding lives in ONE place: UiGroupArgs::read (game/render/ui_group_args.cpp). There used to be
  // an identical readArgs here as well — two decoders of one guest argument shape, which is a silent
  // drift waiting to happen rather than a compile error.
  // Hand one group to the raised page scope (nothing on the oracle / psx_render legs).
  static void route(Core* c, const UiGroupArgs& a);

  // Indices into mGroups in the order the guest's ordering table would paint them.
  std::vector<int> paintOrder() const;
  // Push one collected group to the render queue at `layer` (RQ_OVERLAY for page chrome — one band
  // BELOW the glyph text's RQ_HUD, so chrome can never paint over its own text: the bug #64 /
  // kanban #28 lesson recorded in game/ui/panel.cpp).
  void emit(Core* c, const UiGroupArgs& a, int layer) const;
};
