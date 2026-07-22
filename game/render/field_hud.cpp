// game/render/field_hud.cpp — native producer for the FIELD HUD family (kanban #13).
//
// RE (Ghidra scratch/decomp/f9a8_subs.c + hud_fx_leaves.c + ui_sprite_leaves.c + drawarea_leaf.c):
// the field draw dispatcher FUN_8003F9A8 calls FUN_80025D98 every field frame — the HUD dispatcher
// over the HUD state struct 0x800ED058:
//   FUN_80025744 — status row (equipped-item panel at (0x20,200) + counters + blink select),
//   FUN_80025934 — item ring (data-driven icon list + ring chrome),
//   FUN_80025B78 — equipped-WEAPON strip: prev/current/next weapon icons at y=0xD4, x around
//                  scroll+0xA0, CLIPPED by a DR_AREA window {x=0x90, w=0x20, h=0xE0} (the
//                  32px-wide viewport the carousel scrolls through; DR_AREA BR = x+w-1 per
//                  FUN_80081CF8) — this is the "spiky ball + squiggle" visible in free-roam.
// All three draw through two shared UI leaves:
//   FUN_8007E1B8 — templated POLY_FT4 group (9-word 0x2C/0x2E packets),
//   FUN_8007E6DC — templated SPRT group (op 0x64/0x65/0x66, one DR_TPAGE per group),
// whose MENU-specialized native reproductions already existed (Render::emitMenuFt4/emitMenuSprites,
// validated on the title menu). This file adds the GENERAL forms (caller-supplied template pointer +
// data base + placement overrides + clut/semi attribute) and the field callers.
//
// pc_render never walks the guest OT (break-first rebuild 2026-07-15), so this family had no native
// producer — the weapon strip was entirely absent (evidence: `debug otattr` + REPL `otattr` at the
// seaside field frame 10028 shows its 3 op-0x65 sprites + 2 DR_AREA packets at the head of the
// packet pool, emitted from FUN_8003F9A8's direct-call subtree; the pc-leg render queue has ZERO
// RQ_HUD items at the strip's pixels).
//
// Read-only: reads guest RAM only, emits host-side RQ_HUD quads. Called from Render::renderField()
// (the same field-frame scope FUN_8003F9A8 owns on the guest side).
#include "core.h"
#include "game.h"
#include "cfg.h"
#include "render_internal.h"   // render_queue.h / cur_render_node / render.h
#include "game_ctx.h"          // eng(c) — PauseMenu frame gate
#include "engine.h"

namespace {

// ---- Guest addresses (named per CLAUDE.md; all RE'd from the decomps cited in the banner) ----
constexpr uint32_t kHudState        = 0x800ED058u; // the HUD state struct FUN_80025D98 dispatches on
constexpr uint32_t kHudEnable      = 0x800ED059u; // u8: HUD master enable
constexpr uint32_t kHudSuppress    = 0x800ED060u; // u8: mode-2/7 suppress flag
constexpr uint32_t kHudRingState   = 0x800ED061u; // u8: &3 -> item-ring visibility state
constexpr uint32_t kHudRowEnable   = 0x800ED065u; // u8: status-row enable
constexpr uint32_t kAreaMode       = 0x800BF870u; // u8: field area mode byte
constexpr uint32_t kAreaSub        = 0x800BF871u; // u8: area sub-state
constexpr uint32_t kUiBusy         = 0x800BF816u; // u8: UI-busy latch (dialog/menu up)
constexpr uint32_t kUiFlags822     = 0x800BF822u; // u8: &4 suppresses the status row
constexpr uint32_t kUiFlags880     = 0x800BF880u; // u16: &0x600/&0x100 suppress the weapon strip
constexpr uint32_t kScrollByte87D  = 0x800BF87Du; // u8: item-ring second-loop count
constexpr uint32_t kCount87E       = 0x800BF87Eu; // u8: status-row counter value
constexpr uint32_t kScroll87F      = 0x800BF87Fu; // u8: status-row width scroll
constexpr uint32_t kTaskSmPtr      = 0x1F800138u; // i32: live task state-machine ptr
constexpr uint32_t kOverlayFlag137 = 0x1F800137u; // u8: ==1 suppresses the status row
constexpr uint32_t kFrameCounter   = 0x1F80017Cu; // u32: guest frame counter (blink phase)
constexpr uint32_t kEquipKind      = 0x800E7EEFu; // u8: equipped-item kind (0x12..0x16 ammo classes)
constexpr uint32_t kAmmoThreshBase = 0x800A4554u; // u8[5]: per-kind low-ammo blink thresholds
constexpr uint32_t kRingCount      = 0x800E7FEEu; // s16: item-ring first-loop count
constexpr uint32_t kTemplPtrTable  = 0x80017334u; // u32[]: UI template pointer table (menu + field)
constexpr uint32_t kRingTableBase  = 0x8009D30Cu; // u32[]: per-ring item tables (idx = hudState[10])
constexpr uint32_t kIconTemplIdx   = 0x8009D286u; // s16[] stride 4: weapon icon -> template index
constexpr uint32_t kRowTemplA      = 0x800173D8u; // u32: status-row segment template ptrs
constexpr uint32_t kRowTemplB      = 0x800173DCu;
constexpr uint32_t kSelTemplA      = 0x800173BCu; // u32: counter templates (normal / blink)
constexpr uint32_t kSelTemplB      = 0x800173C0u;
// FUN_80025B78's DR_AREA scroll window: rect {0x90, page<<8, 0x20, 0xE0}; BR = TL + wh - 1.
constexpr int kStripWinX = 0x90, kStripWinW = 0x20, kStripWinH = 0xE0;
constexpr int kStripY    = 0xD4;   // icon row y
constexpr int kStripBaseX = 0xA0;  // current-icon x before the scroll offset (hudState+6)

}  // namespace

// ---- emitUiFt4 — general FUN_8007E1B8 (POLY_FT4 template group) ---------------------------------
// templPtr -> {s16 headerIdx, s16 dataOff} resolved against dataBase; entries 16 bytes:
//   [0,1]=u0,v0 [2,3]=clut [4,5]=u1,v1 [6,7]=tpage [8,9]=u2,v2 [12,13]=u3,v3
//   [10]=w [11]=h [14]=dx(s8) [15]=dy(s8)
// wOv/hOv: >0 replaces the entry size, <0 is added to it, 0 keeps it (the guest's exact rule).
// attrByte: low nibble = layout mode (0 = plain; flip/rotate variants unbuilt -> warn once, skip),
//           high nibble non-zero = modulate by attrByte (&0xF0F0F0), zero = raw texel.
// clutSemi: bit15 = semi-transparent; low 15 bits non-zero = clut override.
void Render::emitUiFt4(int x, int y, int wOv, int hOv, uint32_t templPtr, uint32_t dataBase,
                       uint8_t attrByte, uint16_t clutSemi, int layer,
                       int daX0, int daY0, int daX1, int daY1) {
  Core* c = mCore;
  if (!templPtr || !dataBase) return;
  const int ox = c->game->gpu.s_off_x, oy = c->game->gpu.s_off_y;
  const int  hdr  = (int16_t)c->mem_r16(templPtr);
  const uint32_t psv = dataBase + (uint32_t)hdr * 4u;
  const int  cnt  = (int16_t)c->mem_r16(psv);
  const uint32_t data = dataBase + c->mem_r16(psv + 2u);
  const int mode  = attrByte & 0xF;
  const int raw   = ((attrByte & 0xF0u) == 0) ? 1 : 0;
  const unsigned char col = raw ? 0x80 : (unsigned char)(attrByte & 0xF0u);   // guest: bytes = attr & 0xF0
  const int semi = (clutSemi & 0x8000u) ? 1 : 0;
  // Layout mode (attr low nibble) — vertex arrangement + a ±1 texel inset, transcribed from the
  // FUN_8007E1B8 switch (scratch/decomp/ui_sprite_leaves.c): 0 = plain (TL,TR,BL,BR); 1 = mirror X
  // (u bytes -1); 2 = flip Y (v bytes -1); 3 = 180° rotation (u,v bytes -1); 5 = flip Y (v bytes +1).
  int du = 0, dv = 0;
  switch (mode) {
    case 0: case 1: case 2: case 3: case 5: break;
    default: {
      static bool warned = false;
      if (!warned) { warned = true;
        cfg_logw("fieldhud", "emitUiFt4 layout mode %d not built — group skipped (templ=%08X)", mode, templPtr); }
      return;
    }
  }
  if (mode == 1 || mode == 3) du = -1;
  if (mode == 2 || mode == 3) dv = -1;
  if (mode == 5)              dv = 1;
  // Entries are walked BACK-TO-FRONT, exactly like emitUiSprites: the guest links each packet into
  // its OT bucket with AddPrim, which prepends, so the LAST entry of a group is drawn FIRST and
  // entry 0 ends up on top. Measured on the pause menu's help-panel portrait (kanban #21): the four
  // overlapping pieces walk 0x800BFFD0 -> FF08 -> FEB8 -> FE68 in the psx_render OT, i.e. strictly
  // DESCENDING packet-pool address = descending k, and emitting them ascending put the wrong piece
  // on top. Only matters where a group's entries overlap, which is why it went unnoticed on the
  // field HUD's non-overlapping tile rows.
  const int n = (cnt < 16) ? cnt : 16;
  for (int k = n - 1; k >= 0; k--) {
    const uint32_t e = data + (uint32_t)k * 16u;
    int w = c->mem_r8(e + 10u), h = c->mem_r8(e + 11u);
    if (wOv > 0) w = wOv; else if (wOv < 0) w += wOv;
    if (hOv > 0) h = hOv; else if (hOv < 0) h += hOv;
    const int x0 = x + (int8_t)c->mem_r8(e + 14u);
    const int y0 = y + (int8_t)c->mem_r8(e + 15u);
    uint16_t clut = c->mem_r16(e + 2u);
    if (clutSemi & 0x7FFFu) clut = clutSemi;
    const uint16_t tpage = c->mem_r16(e + 6u);
    const int XL = x0 + ox, XR = x0 + w + ox, YT = y0 + oy, YB = y0 + h + oy;
    int xs[4], ys[4];
    switch (mode) {
      case 0:  xs[0]=XL; xs[1]=XR; xs[2]=XL; xs[3]=XR; ys[0]=YT; ys[1]=YT; ys[2]=YB; ys[3]=YB; break;
      case 1:  xs[0]=XR; xs[1]=XL; xs[2]=XR; xs[3]=XL; ys[0]=YT; ys[1]=YT; ys[2]=YB; ys[3]=YB; break;
      case 2:
      case 5:  xs[0]=XL; xs[1]=XR; xs[2]=XL; xs[3]=XR; ys[0]=YB; ys[1]=YB; ys[2]=YT; ys[3]=YT; break;
      default: xs[0]=XR; xs[1]=XL; xs[2]=XR; xs[3]=XL; ys[0]=YB; ys[1]=YB; ys[2]=YT; ys[3]=YT; break;   // 3
    }
    int us[4] = { (c->mem_r8(e+0u)+du)&0xFF, (c->mem_r8(e+4u)+du)&0xFF, (c->mem_r8(e+8u)+du)&0xFF,  (c->mem_r8(e+12u)+du)&0xFF };
    int vs[4] = { (c->mem_r8(e+1u)+dv)&0xFF, (c->mem_r8(e+5u)+dv)&0xFF, (c->mem_r8(e+9u)+dv)&0xFF,  (c->mem_r8(e+13u)+dv)&0xFF };
    unsigned char cc[4] = { col, col, col, col };
    const int tp_x = (tpage & 0xF) * 64, tp_y = ((tpage >> 4) & 1) * 256;
    const int tmode = (tpage >> 7) & 3, blend = (tpage >> 5) & 3;
    const int clut_x = (clut & 0x3F) * 16, clut_y = (clut >> 6) & 0x1FF;
    c->game->activeRq().emitOrQueue(c, /*capture=*/1, layer, RQ_OM_2D_FG, 4, semi, raw,
                                    xs, ys, nullptr, nullptr, us, vs, cc, cc, cc, nullptr,
                                    tmode, tp_x, tp_y, clut_x, clut_y,
                                    0, 0, 0, 0, daX0, daY0, daX1, daY1, blend);
  }
}

// ---- emitUiSprites — general FUN_8007E6DC (SPRT template group) ---------------------------------
// Entry layout matches emitUiFt4 with a single texture corner ([0,1]=u,v). One tpage for the whole
// group from entry 0's [6,7]. Groups are authored FRONT-FIRST (see emitMenuSprites) — walked
// back-to-front so queue order gives the same layering.
void Render::emitUiSprites(int x, int y, uint32_t templPtr, uint32_t dataBase,
                           uint8_t attrByte, uint16_t clutSemi, int layer,
                           int daX0, int daY0, int daX1, int daY1) {
  Core* c = mCore;
  if (!templPtr || !dataBase) return;
  const int ox = c->game->gpu.s_off_x, oy = c->game->gpu.s_off_y;
  const int  hdr  = (int16_t)c->mem_r16(templPtr);
  const uint32_t psv = dataBase + (uint32_t)hdr * 4u;
  const int  cnt  = (int16_t)c->mem_r16(psv);
  const uint32_t data = dataBase + c->mem_r16(psv + 2u);
  const uint16_t tpage = c->mem_r16(data + 6u);
  const int raw   = ((attrByte & 0xF0u) == 0) ? 1 : 0;
  const unsigned char col = raw ? 0x80 : (unsigned char)(attrByte & 0xF0u);
  const int semi = (clutSemi & 0x8000u) ? 1 : 0;
  const int tp_x = (tpage & 0xF) * 64, tp_y = ((tpage >> 4) & 1) * 256;
  const int tmode = (tpage >> 7) & 3, blend = (tpage >> 5) & 3;
  const int n = (cnt < 16) ? cnt : 16;
  for (int k = n - 1; k >= 0; k--) {
    const uint32_t e = data + (uint32_t)k * 16u;
    const int x0 = x + (int8_t)c->mem_r8(e + 14u);
    const int y0 = y + (int8_t)c->mem_r8(e + 15u);
    const int w  = c->mem_r8(e + 10u), h = c->mem_r8(e + 11u);
    const int u0 = c->mem_r8(e + 0u),  v0 = c->mem_r8(e + 1u);
    uint16_t clut = c->mem_r16(e + 2u);
    if (clutSemi & 0x7FFFu) clut = clutSemi;
    int xs[4] = { x0 + ox, x0 + w + ox, x0 + ox,     x0 + w + ox };
    int ys[4] = { y0 + oy, y0 + oy,     y0 + h + oy, y0 + h + oy };
    int us[4] = { u0, u0 + w, u0,     u0 + w };
    int vs[4] = { v0, v0,     v0 + h, v0 + h };
    unsigned char cc[4] = { col, col, col, col };
    const int clut_x = (clut & 0x3F) * 16, clut_y = (clut >> 6) & 0x1FF;
    c->game->activeRq().emitOrQueue(c, /*capture=*/1, layer, RQ_OM_2D_FG, 4, semi, raw,
                                    xs, ys, nullptr, nullptr, us, vs, cc, cc, cc, nullptr,
                                    tmode, tp_x, tp_y, clut_x, clut_y,
                                    0, 0, 0, 0, daX0, daY0, daX1, daY1, blend);
  }
}

// ---- FUN_80025744 — status row ------------------------------------------------------------------
void Render::fieldHudStatusRow() {
  Core* c = mCore;
  const uint32_t p = kHudState;
  const uint32_t base = c->mem_r32(p + 0x3Cu);
  // 1: equipped-item panel at (0x20, 200), semi, template ptr straight from the struct (+0x38).
  emitUiFt4(0x20, 200, 0, 0, c->mem_r32(p + 0x38u), base, 0, 0x8000u, RQ_HUD);
  // 2+3: the two row segments; segment A width-overridden by the scroll byte - 0x18.
  const int wA = (int)c->mem_r8(kScroll87F) - 0x18;
  emitUiFt4(0x20 + 0x18, 200, wA, 0, c->mem_r32(kRowTemplA), base, 0, 0, RQ_HUD);
  emitUiFt4(0x20 + 0x18 + wA, 200, 0, 0, c->mem_r32(kRowTemplB), base, 0, 0, RQ_HUD);
  // 4: counter at (0x32, 0xCC), width = the count; blinks when ammo is at/below half threshold.
  const uint8_t count = c->mem_r8(kCount87E);
  if (!count) return;
  bool low = false;
  const uint8_t kind = c->mem_r8(kEquipKind);
  if (kind >= 0x12 && kind <= 0x16) {
    const uint8_t thresh = c->mem_r8(kAmmoThreshBase + (kind - 0x12u));
    low = (thresh >> 1) <= count;
  }
  const bool blink = low && (c->mem_r32(kFrameCounter) & 3u);
  emitUiFt4(0x32, 0xCC, count, 0, c->mem_r32(blink ? kSelTemplB : kSelTemplA), base, 0, 0, RQ_HUD);
}

// ---- FUN_80025934 — item ring -------------------------------------------------------------------
void Render::fieldHudItemRing(int offsetMode, uint32_t /*bucketAttr*/) {
  Core* c = mCore;
  const uint32_t p = kHudState;
  const uint32_t base = c->mem_r32(p + 0x3Cu);
  const int dx = offsetMode ? 0x10 : 0, dy = offsetMode ? 0x28 : 0;
  const uint32_t tab = c->mem_r32(kRingTableBase + (uint32_t)c->mem_r8(p + 10u) * 4u);
  const int n1 = (int16_t)c->mem_r16(kRingCount);
  const int n2 = (int)c->mem_r8(kScrollByte87D);
  for (int i = 0; i < n1; i++) {
    const uint32_t e = tab + (uint32_t)i * 4u;
    emitUiFt4(c->mem_r8(e + 1u) + dx, c->mem_r8(e + 2u) + dy, 0, 0,
              c->mem_r32(kTemplPtrTable + (uint32_t)c->mem_r8(e) * 4u), base,
              c->mem_r8(e + 3u), 0, RQ_HUD);
  }
  for (int i = n1; i < n2; i++) {
    const uint32_t e = tab + (uint32_t)i * 4u;
    emitUiFt4(c->mem_r8(e + 1u) + dx, c->mem_r8(e + 2u) + dy, 0, 0,
              c->mem_r32(kTemplPtrTable + ((uint32_t)c->mem_r8(e) + 1u) * 4u), base,
              c->mem_r8(e + 3u), 0, RQ_HUD);
  }
  // Ring chrome: one sprite group + two semi FT4 groups at (dx+0x20, dy+0x20).
  const int rx = dx + 0x20, ry = dy + 0x20;
  emitUiSprites(rx, ry, c->mem_r32(kTemplPtrTable + ((uint32_t)(int16_t)c->mem_r16(kRingCount) + 0x11u) * 4u),
                base, 0, 0, RQ_HUD);
  emitUiFt4(rx, ry, 0, 0, c->mem_r32(kTemplPtrTable + 4u * 4u), base, 0, 0x8000u, RQ_HUD);
  emitUiFt4(rx, ry, 0, 0, c->mem_r32(kTemplPtrTable + 3u * 4u), base, 0, 0x8000u, RQ_HUD);
}

// ---- FUN_80025B78 — equipped-weapon strip (the kanban #13 layer) --------------------------------
void Render::fieldHudWeaponStrip() {
  Core* c = mCore;
  const uint32_t p = kHudState;
  const uint32_t base = c->mem_r32(p + 0x3Cu);
  const int oy = c->game->gpu.s_off_y;
  // The guest brackets the icons between a full-viewport DR_AREA reset and the 32px scroll-window
  // DR_AREA {0x90, page<<8, 0x20, 0xE0}; in OT-bucket LIFO order the WINDOW applies to the icons.
  // Native: the same window as per-prim clip rect (BR inclusive = TL + wh - 1, per FUN_80081CF8).
  const int daX0 = kStripWinX, daX1 = kStripWinX + kStripWinW - 1;
  const int daY0 = oy, daY1 = oy + kStripWinH - 1;
  auto icon = [&](int x, uint32_t slot) {
    const uint8_t weapon = c->mem_r8(p + slot + 0x22u);
    const int templIdx = (int16_t)c->mem_r16(kIconTemplIdx + (uint32_t)weapon * 4u);
    emitUiSprites(x, kStripY, c->mem_r32(kTemplPtrTable + (uint32_t)templIdx * 4u), base,
                  0, 0, RQ_HUD, daX0, daY0, daX1, daY1);
  };
  const uint32_t cur = c->mem_r8(p + 8u);
  int x = (int8_t)c->mem_r8(p + 6u) + kStripBaseX;
  icon(x, cur);                                     // current
  int prev = (int)cur - 1;
  if (prev < 0) prev = (int)c->mem_r8(p + 7u) - 1;
  x -= 0x20;
  icon(x, (uint32_t)prev);                          // previous
  uint32_t next = cur + 1u;
  if (c->mem_r8(p + 7u) <= next) next = 0;
  x += 0x40;
  icon(x, next);                                    // next
}

// ---- FUN_80025D98 — the HUD dispatcher gate (transcribed 1:1) -----------------------------------
void Render::fieldHudRender() {
  Core* c = mCore;
  // While the in-game pause/item menu is up the guest replaces the whole frame with menu chrome —
  // no world, no HUD (measured: all 421 ordering-table packets that frame come from the menu
  // controller FUN_800346BC). PauseMenu is the native producer for that frame; the field HUD stands
  // down, exactly as the guest's own HUD dispatcher does.
  if (eng(c).pauseMenu.upThisFrame()) return;
  const uint8_t mode = c->mem_r8(kAreaMode);
  const uint8_t sub  = c->mem_r8(kAreaSub);
  const bool subEarly = (uint8_t)(sub - 1u) <= 2u;   // guest: 2 < sub-1 means NOT early

  if (c->mem_r8(kHudRowEnable) != 0 &&
      (mode != 5 || !subEarly) && mode != 3 && mode != 0x14 &&
      c->mem_r8(kOverlayFlag137) != 1 && !(c->mem_r8(kUiFlags822) & 4u))
    fieldHudStatusRow();

  const uint8_t ring = c->mem_r8(kHudRingState) & 3u;
  bool ringDrawn = false;
  if (ring == 1) {
    if ((mode != 5 || !subEarly) && mode != 2 && mode != 7 && mode != 0x14) {
      if (c->mem_r8(kHudEnable) == 0) goto tail;
      fieldHudItemRing(0, 0); ringDrawn = true;
    }
  } else if (ring == 3) {
    fieldHudItemRing(0, 0); ringDrawn = true;
  }
  (void)ringDrawn;

  if (c->mem_r8(kHudEnable) == 0 || mode == 3) goto tail;
  if (mode == 2 || mode == 7) goto tail;   // overlay-resident drawers 0x80113628/0x801140A0 — unowned, draw nothing
  if (mode == 0x14) goto tail;
  if (!(c->mem_r16(kUiFlags880) & 0x600u) && !(c->mem_r16(kUiFlags880) & 0x100u) &&
      c->mem_r8(kUiBusy) == 0)
    fieldHudWeaponStrip();

tail:
  // task-sm[0x4C]==6 special pages (0x801121AC / 0x8010F8CC) are overlay-resident and unowned —
  // nothing drawn natively (honest gap, tracked in docs/findings/render.md).
  return;
}
