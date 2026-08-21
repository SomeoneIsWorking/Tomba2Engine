// class UiGroupCapture — a page's ONE ORDERED CHROME LIST: everything the guest linked into the
// ordering table for that page, drained in the order the ordering table itself would paint it.
//
// WHY IT EXISTS. The 2D chrome leaves are game-wide — the templated group emitters FUN_8007E1B8
// (POLY_FT4) and FUN_8007E6DC (SPRT), and the panel builders FUN_8005019C / FUN_8004FFB4 — so the
// pause/item menu (kanban #21), the in-game START page (#35), the OPTIONS pages (#38), the
// memory-card menu (#102) and the field HUD all reach the same four. Each leaf is owned ONCE — a
// second overrides::install on one address is the dual-ownership bug that broke the dialog box
// (kanban #28) — so the taps live where the address is owned (ui_ft4_tap.cpp, ui_sprite.cpp,
// panel.cpp) and hand their arguments to `route`, which files them under whichever PAGE SCOPE is
// currently raised. The GAME.BIN Save/Continue machine is classified as its own scope and routes
// directly through this same emitter; other unscoped calls belong to independent producers such as
// the field HUD and dialog box.
//
// ONE LIST, NOT ONE PER EMITTER — this is the point, and it is what the card menu forced.
// A page's picture is ONE ordering table. When each tap chose its own host layer instead, a page's
// stacking became a per-producer guess: the card menu's backdrop grid painted OVER the slot rows at
// RQ_OVERLAY and vanished UNDER the field at RQ_BACKGROUND (both measured), and OptionsPage had
// already grown a private box list to hand-order the same conflict. So panels and groups go into the
// SAME list here, each carrying the bucket the guest gave it, and the page drains all of it at ONE
// layer. A render bug on a page is then a question about the guest's data, never about which host
// layer to try.
//
// PAINT ORDER COMES FROM THE GUEST'S ORDERING TABLE, NOT FROM CALL ORDER. `paintOrder` sorts by
// DESCENDING OT bucket (attrs+1 for a group, the `otBucket` argument for a panel — a lower PSX OT
// index is walked later, i.e. drawn in front) and, within one bucket, in REVERSE call order (AddPrim
// prepends, so a bucket's list is LIFO). Emitting in plain call order is wrong and visibly so —
// measured on the pause menu, where the black panel interior painted over the item icons.
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

// One thing the guest linked into a page's ordering table. `otBucket` is the sort key for EVERY kind
// — that is what lets a panel and a sprite group be ordered against each other at all.
struct PageChromeItem {
  enum class Kind : uint8_t { Group, PanelFill, PanelCorners };
  Kind kind = Kind::Group;
  uint8_t otBucket = 0;

  UiGroupArgs group{}; // Kind::Group — FUN_8007E1B8 / FUN_8007E6DC

  // Panel kinds — the arguments of FUN_8004FFB4 (fill) and FUN_8005019C (corners), with the RECT
  // RESOLVED TO VALUES. The guest passes those builders a pointer into the CALLER'S STACK FRAME, so
  // a deferred panel must read it while that frame is still live; holding the pointer instead lost
  // most of the memory-card menu's slot panels, because the guest had reused the stack by drain
  // time. Everything else stays an argument so the geometry keeps ONE implementation
  // (Panel::pushFillAt / pushCornersAt), inline or deferred.
  int rx = 0, ry = 0, rw = 0, rh = 0;
  int32_t uvIndex = 0; // PanelFill
  uint16_t attr = 0;   // PanelFill: attr.  PanelCorners: style.
  uint32_t shadow = 0; // PanelCorners
};

class UiGroupCapture {
public:
  // Raised for the duration of one page controller (see the scope wrappers in pause_menu.cpp /
  // start_page.cpp / card_menu.cpp). Only while it is up does `route` record anything here.
  bool mCapturing = false;
  // Everything the page linked into its ordering table this frame, in CALL order. Re-ordered by
  // paintOrder, drained by the owning producer.
  std::vector<PageChromeItem> mItems;

  bool capturing() const {
    return mCapturing;
  }
  void begin() {
    mCapturing = true;
  }
  void end() {
    mCapturing = false;
  }
  void clear() {
    mItems.clear();
  }
  bool empty() const {
    return mItems.empty();
  }

  // The ONE way in, so a page never pushes into mItems itself and no caller can forget the bucket.
  void fileGroup(const UiGroupArgs &a);
  void filePanelFill(Core *c, uint32_t rectPtr, int32_t uvIndex, uint16_t attr, int32_t otBucket);
  void filePanelCorners(Core *c, uint32_t rectPtr, uint16_t style, uint32_t shadow, int32_t otBucket);

  // Decoding lives in ONE place: UiGroupArgs::read (game/render/ui_group_args.cpp). There used to be
  // an identical readArgs here as well — two decoders of one guest argument shape, which is a silent
  // drift waiting to happen rather than a compile error.
  // route*: hand one item to the raised page scope. Nothing on the oracle / psx_render legs. The
  // panel forms return TRUE when a scope took the item, so the tap knows to skip its inline push —
  // the item is not dropped, it is DEFERRED into the page's ordered drain.
  static void route(Core *c, const UiGroupArgs &a);
  static bool routePanelFill(Core *c, uint32_t rectPtr, int32_t uvIndex, uint16_t attr, int32_t otBucket);
  static bool routePanelCorners(Core *c, uint32_t rectPtr, uint16_t style, uint32_t shadow, int32_t otBucket);

  // Indices into mItems in the order the guest's ordering table would paint them.
  std::vector<int> paintOrder() const;
  // Draw one collected item at `layer` (RQ_OVERLAY for page chrome — one band BELOW the glyph text's
  // RQ_HUD, so chrome can never paint over its own text: the bug #64 / kanban #28 lesson recorded in
  // game/ui/panel.cpp).
  void emit(Core *c, const PageChromeItem &it, int layer) const;
};
