// class ScorePopup — native display producer for the SCORE / AP-GEM PICKUP POPUP (kanban #18).
//
// THE GUEST SIDE. Walking into an AP gem runs the collectible's behavior FUN_8004C238
// (beh_visibility_gate_dispatch, owned) -> Spawn::dropScoreGem (0x8004B3F4, owned) -> the popup
// SPAWNER FUN_80071B44, which allocates an entity via Placement::spawnWithParent(src,0x80,3,0xd),
// installs the per-frame handler FUN_80072520 at node+0x1C, renders the value to an ASCII decimal
// string at node+0x44 (digit count at node+0xE, via FUN_80079634), picks the CLUT at node+0x5C
// (0x7C7E under 5000 / 0x7C3E at or above), and publishes the live popup pointer at 0x800BF83C.
//
// FUN_80072520 is the popup's whole life: state 0 seeds a 60-frame timer at node+0x40, state 1
// draws — FUN_80071DFC for the SCORE-DIGIT popup (node+3 < 2) and FUN_80072308 for the
// item/count sibling (node+3 == 2) — state 3 despawns. Both drawers load the pure camera from
// scratchpad 0x1F8000F8, anchor on the player (0x800E7EAE / 0x800E7EB2 - 200, or -0x8C when
// 0x800E7FF4 & 4 / 0x800E7EB6), project that anchor with FUN_8003F7A0, and then emit ONE templated
// POLY_FT4 GROUP PER GLYPH through the shared leaf FUN_8007E1B8. Those groups are the whole picture.
//
// WHY IT WAS INVISIBLE. pc_render does not walk the guest OT (break-first render rebuild
// 2026-07-15), and the only native tap on FUN_8007E1B8 was the pause menu's, which records nothing
// outside its own scope. So the guest kept building the packets and the layer was simply lost.
// Measured (2026-07-22, AP-gem collect forced on the AUTO_SKIP field, box rows 60..95 cols 90..140,
// yellow glyph pixels): psx_render leg 60, pc_render leg 0 — the "100" floats over Tomba on the psx
// leg and nothing at all is drawn on the pc leg.
//
// THE PRODUCER = A SCOPED LEAF TAP, the same shape as PauseMenu (kanban #21). popupTick overrides
// FUN_80072520 with a wrapper: raise a per-Core capture scope, run the UNTOUCHED gen body, lower it,
// then draw what the scope collected. One scope covers both sub-drawers and no other caller, so the
// field HUD's and the pause menu's own FUN_8007E1B8 traffic can never leak in. Read-only: host
// memory only, not one guest write outside gen.
//
// ORDER COMES FROM THE GUEST'S OWN SORT KEY, NOT FROM CALL ORDER: descending OT bucket (attrs+1 — a
// lower PSX OT index is walked later, i.e. drawn in front), and within one bucket in REVERSE call
// order because AddPrim prepends. The digits emit left-to-right, so plain call order is wrong.
//
// LAYER: RQ_OVERLAY, above the world and one band below the glyph text's RQ_HUD — the bug #64 /
// kanban #28 lesson recorded in game/ui/panel.cpp.
#pragma once
#include <cstdint>
#include <vector>
#include "ui_group_args.h"
class Core;

class ScorePopup {
public:
  Core* core = nullptr;

  // Raised for the duration of the guest popup handler FUN_80072520 (see popupTick). Per-Core (never
  // a file-scope flag) so SBS's two cores cannot see each other's scope.
  bool mInDraw = false;
  // Groups collected during one handler run, in call order. Drained + re-ordered by drawCollected.
  std::vector<UiGroupArgs> mGroups;

  // install(): registers the FUN_80072520 scope wrapper. The FUN_8007E1B8 leaf tap that feeds
  // collect() is owned by UiFt4Tap (one install per address — see ui_ft4_tap.h). Idempotent.
  static void install();

  // collect: record one group if the popup scope is up and this is the pc_render leg. Called from
  // the shared leaf taps.
  static void collect(Core* c, const UiGroupArgs& a);

  // drawCollected: sort the collected groups into the guest's paint order and push them to the
  // render queue at RQ_OVERLAY. Called once per handler run, by popupTick.
  void drawCollected();
};
