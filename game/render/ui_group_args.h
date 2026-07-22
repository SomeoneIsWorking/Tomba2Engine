// UiGroupArgs — the argument shape the game's two TEMPLATED UI GROUP LEAVES share.
//
//   FUN_8007E1B8 — POLY_FT4 group (9-word 0x2C/0x2E packets), host reproduction Render::emitUiFt4
//   FUN_8007E6DC — SPRT group (op 0x64/0x65/0x66),           host reproduction Render::emitUiSprites
//
// Both take (a0 = placement record, a1 = template pointer, a2 = definition base, a3 = attributes).
// Every native producer that taps one of those leaves decodes the SAME four registers, so the decode
// lives here once instead of being re-derived per producer (it was PauseMenu::GroupArgs until the
// score popup, kanban #18, became the second scope to need it).
#pragma once
#include <cstdint>
class Core;

struct UiGroupArgs {
  int      x, y;        // placement +0 / +2 — screen anchor of the group
  int      wOv, hOv;    // placement +4 / +6 — size override: >0 replaces, <0 adds, 0 keeps
  uint32_t templPtr;    // r5 — s16 index into the definition table at dataBase
  uint32_t dataBase;    // r6 — definition + piece-entry base
  uint8_t  attrByte;    // attrs +0 — low nibble = layout mode, high nibble = flat shade level
  uint16_t clutSemi;    // attrs +2 — bit15 = semi-transparent, low 15 bits = CLUT override
  uint8_t  otBucket;    // attrs +1 — the ordering-table bucket, i.e. the guest's own sort key
  bool     sprite;      // false = FT4 group (FUN_8007E1B8), true = SPRT group (FUN_8007E6DC)

  // Decode the guest-ABI arguments of either leaf. MUST be called BEFORE the guest body runs: both
  // leaves rewrite attrs+0 as (flags & 0x0F), so the shade nibble is gone afterwards.
  static UiGroupArgs read(Core* c, bool sprite);
};
