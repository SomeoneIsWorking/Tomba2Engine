// game/render/ui_ft4_layout.h — the POLY_FT4 UI-group emitter's VERTEX-LAYOUT case blocks.
//
// These are NOT standalone functions. FUN_8007E1B8 stages one 0x2C-byte POLY_FT4 packet in the
// scratchpad at 0x1F800000, then reads a layout mode from the low nibble of a descriptor byte and
// rec_dispatches into one of six case blocks through a table at 0x8001728C. The recompiler split
// each case out as its own gen body because rec_dispatch is the only way in. This class owns them.
//
// Distinct from Render::emitUiFt4 (game/render/field_hud.cpp), which re-derives the same six layouts
// in float space as a HOST-ONLY pc_render producer and writes no guest memory. That one draws the
// picture; these write the guest packet the substrate's renderer consumes.
#pragma once
struct Core;
class  Game;

class UiFt4Layout {
public:
  // FUN_8007E2F8 — layout mode 0, the PLAIN (unflipped) corner arrangement:
  //   XY0=(x,y)  XY1=(x+w,y)  XY2=(x,y+h)  XY3=(x+w,y+h), with a per-entry signed s8 nudge applied
  // to x and y first. The sibling at 0x8007E36C writes the same packet mirrored (x-corners swapped,
  // every u decremented), which is what makes "plain" the discriminating fact rather than a guess.
  //
  // TAIL-JUMPS into func_8007E620 (the shared "link the packet into the OT and advance" tail) and is
  // RE-ENTERED once per quad in the group. Nothing arrives in a0..a3 — the parent leaves its state in
  // r5/r6/r8/r9/r12 — and nothing is returned.
  //
  // abi_extract reports `r[31] = MISSING` at the call. That is a FALSE POSITIVE and must not be
  // "fixed": this is a `j`, not a `jal`. r31 has to keep the value it arrived with, which is
  // FUN_8007E1B8's caller's return address. Writing an ra constant here would return into the middle
  // of this block. port_gen emits it absent, which is correct.
  static void plainQuadVerts(Core* c);

  // FUN_0x8007E36C — layout mode 1 — X-MIRRORED. Base XY goes to VERTEX 1 rather than 0 and the width is added to the
  static void xMirroredQuadVerts(Core* c);
  // FUN_0x8007E410 — layout mode 2.
  static void vMirroredQuadVerts(Core* c);
  // FUN_0x8007E4A8 — layout mode 3.
  static void flipXYQuadVerts(Core* c);
  // FUN_0x8007E584 — layout mode 4.
  static void vMirroredPlusQuadVerts(Core* c);

  static void registerOverrides(Game* game);
};
