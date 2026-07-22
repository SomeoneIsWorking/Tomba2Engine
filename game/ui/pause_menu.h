// class PauseMenu — native display producer for the IN-GAME PAUSE / ITEM MENU (kanban #21).
//
// THE GUEST SIDE. The triangle-button menu is driven by the GAME-overlay page dispatcher
// FUN_8010810C (owned: Engine::submitPage810c), whose page-1 branch calls the still-recomp menu
// driver FUN_801084F8, which in turn runs the menu CONTROLLER FUN_800346BC. That controller owns
// the whole page family (Items / Event / Status / Help — its sub-drawers 0x80034548, 0x80037E44,
// 0x80038A00, 0x80039110, 0x80039BCC) and paints every piece of chrome through two shared leaves:
// the templated POLY_FT4 group emitter FUN_8007E1B8 and its SPRT sibling FUN_8007E6DC, whose
// host-side reproductions we already own as Render::emitUiFt4 / Render::emitUiSprites
// (game/render/field_hud.cpp).
//
// WHY IT WAS INVISIBLE. pc_render does not walk the guest OT (break-first render rebuild
// 2026-07-15), and nothing natively produced this family — so the guest kept building the packets
// and the picture simply lost the layer. Measured (2026-07-22, menu open on the seaside field at
// frame 323):
//   * psx_render leg: `PSXPORT_PRIMAT="160,120,323"` reports ONE gp0 prim over the panel centre —
//     `op=2D semi=0 raw=1 tp=(384,0) clut=(480,249) uv0=(120,120) bbox=(28,51)-(292,180)`, i.e. the
//     opaque panel interior; `debug otattr` attributes it (and 194 sibling op-0x2D packets) to
//     fn=0x8007E2F8 (inside FUN_8007E1B8) under caller=0x800346BC.
//   * pc_render leg: the SAME PRIMAT probe reports no menu prim at that pixel at all — only world
//     geometry. So the opaque slice IS submitted; it had no native producer.
//
// THE PRODUCER = A SCOPED LEAF TAP. `menuTick` overrides FUN_800346BC with a wrapper: raise a
// per-Core capture scope, run the untouched gen body, lower it, then draw what the scope collected.
// `uiFt4Tap` overrides FUN_8007E1B8 (and ui_sprite.cpp's ov_compose does the same for FUN_8007E6DC):
// run the untouched guest body, then — only inside that scope, and only on the pc_render leg —
// record the SAME guest-ABI arguments. Read-only: host memory only, not one guest write outside gen.
//
// The scope is what keeps this from double-drawing. Both leaves are game-wide, and the field HUD
// family (Render::fieldHudRender, kanban #13) already produces ITS groups by calling emitUiFt4
// directly — those calls happen outside FUN_800346BC, so the tap never sees them.
//
// ORDER COMES FROM THE GUEST'S OWN SORT KEY, NOT FROM CALL ORDER. Groups are collected, then drawn
// in the order the guest's ordering table would paint them: DESCENDING OT bucket (attrs+1 — a lower
// PSX OT index is walked later, i.e. drawn in front), and within one bucket in REVERSE call order
// (AddPrim prepends, so the list is LIFO). Emitting in plain call order is wrong and visibly so: the
// item icons are bucket 103 but are emitted BEFORE the panel interior (bucket 107), so a call-order
// queue paints the black panel over them — measured, the icons and the green divider vanished.
//
// LAYER: RQ_OVERLAY, one band BELOW the glyph text's RQ_HUD. The queue sorts (layer, seq), so the
// chrome always composites UNDER the menu text no matter which emitter runs later — the bug #64 /
// kanban #28 lesson recorded in game/ui/panel.cpp.
#pragma once
#include <cstdint>
#include <vector>
class Core;

class PauseMenu {
public:
  Core* core = nullptr;

  // FUN_8007E1B8 / FUN_8007E6DC share one argument shape: a placement record, a template index, a
  // definition base and a little attribute struct. RE'd from gen_func_8007E1B8 (shard_2) and
  // scratch/decomp/ui_sprite_leaves.c; named here so the taps read as calls rather than as a pile
  // of mem_r16(reg + n).
  struct GroupArgs {
    int      x, y;        // placement +0 / +2 — screen anchor of the group
    int      wOv, hOv;    // placement +4 / +6 — size override: >0 replaces, <0 adds, 0 keeps
    uint32_t templPtr;    // r5 — s16 index into the definition table at dataBase
    uint32_t dataBase;    // r6 — definition + piece-entry base
    uint8_t  attrByte;    // attrs +0 — low nibble = layout mode, high nibble = flat shade level
    uint16_t clutSemi;    // attrs +2 — bit15 = semi-transparent, low 15 bits = CLUT override
    uint8_t  otBucket;    // attrs +1 — the ordering-table bucket, i.e. the guest's own sort key
    bool     sprite;      // false = FT4 group (FUN_8007E1B8), true = SPRT group (FUN_8007E6DC)
  };

  // Raised for the duration of the guest menu controller FUN_800346BC (see menuTick). Only while it
  // is up do the two leaf taps record anything, so the field HUD's own emitUiFt4 callers are
  // untouched. Per-Core (never a file-scope flag) so SBS's two cores cannot see each other's scope.
  bool mInMenuDraw = false;
  // Groups collected during one controller run, in call order. Drained + re-ordered by drawCollected.
  std::vector<GroupArgs> mGroups;
  // Guest frame counter (0x1F80017C) of the last frame the menu actually drew. While the menu is up
  // the guest submits NOTHING else — the whole ordering table that frame is menu chrome — so the
  // field HUD's native producer must stand down too, or its icons paint over the menu (measured: a
  // 32x24 quad from the menu texpage sat on the help panel). See Render::fieldHudRender.
  uint32_t mLastDrawFrame = 0xFFFFFFFFu;
  bool upThisFrame() const;

  // install(): registers the FUN_800346BC scope wrapper and the FUN_8007E1B8 tap on the main-EXE
  // table. Idempotent; called from games_tomba2_init alongside the other *_install() wirings.
  static void install();

  // Both leaves rewrite attrs+0 as (flags & 0x0F), so the shade nibble must be read BEFORE the
  // guest body runs — the same reason UiSprite::compose reads its attributes up front.
  static GroupArgs readGroupArgs(Core* c, bool sprite);

  // collect: record one group if the menu scope is up and this is the pc_render leg. Called by both
  // taps. FUN_8007E6DC's address is owned by UiSprite::compose (game/ui/ui_sprite_compose.cpp — a
  // full port, not a gen call), so ITS tap lives in ui_sprite.cpp's ov_compose and calls this; a
  // second overrides::install on one address is the dual-ownership bug that broke the dialog box
  // (kanban #28).
  static void collect(Core* c, const GroupArgs& a);

  // drawCollected: sort the collected groups into the guest's paint order and push them, plus the
  // menu's own two full-screen quads, to the render queue at RQ_OVERLAY. Called once per controller
  // run, by menuTick.
  void drawCollected();

  // pushScreenQuad / pushSubtractiveDim: the menu's two untextured 320x240 quads — the opaque black
  // backdrop it starts from, and the subtractive dim it lays over the outer chrome. Both are guest
  // packets (GP0 0x60 / 0xE1+0x62); see drawCollected for the read-back that fixes their colours and
  // their place in the paint order.
  void pushScreenQuad(unsigned char level, int semi, int blend);
  void pushSubtractiveDim();
};
