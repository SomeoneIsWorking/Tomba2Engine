// game/render/fx_sprite.h — the SHARED contract of the FUN_80027A4C world-anchored sprite family.
//
// Three producers now draw from this family (fx_sprite.cpp's flames, fx_sprite.cpp's animated four-corner
// puffs, fx_vortex.cpp's area-15 portal), and every one of them needs the same two RE'd relations the
// guest emitters set up before they RTPS an anchor:
//   * the depth-cue-as-scale trick — the emitters program DQA to a small integer and DQB to 0, so the
//     GTE's MAC0 after RTPS is a per-Z PIXEL SCALE rather than a fog factor;
//   * the OT-bucket RANGE GATE on (SZ3>>2)+bias, which is the emitter's own emit/skip decision.
// They live here as static methods so a second producer cannot drift from the first.
#pragma once
#include <stdint.h>

class SpriteAnchor {
public:
  // MAC0 = n·DQA with n = (H·0x20000/SZ3 + 1)/2 — the RTPS perspective divide, integer-faithful (the
  // GTE's UNR reciprocal differs by <1 ULP and never changes a pixel here).
  static int32_t baseScale(uint32_t H, int sz, int dqa);

  // The emitter's OT-key range gate, reproduced on the native SZ3: logarithmic bucket map, valid [4,0x7FF].
  static bool otKeyInRange(int sz, int bias);

  // GTE DPCS (opcode 0x780010, sf=1 lm=0) on one 8-bit colour component: the packet writer runs every
  // record colour through the depth cue with IR0 = *(0x1F800090) and the caller's far colour, and stores
  // the RESULT into the packet. The flame emitters force IR0 = 0 (the cue is then the identity), but the
  // portal drives it hard — its particles fade toward a blue far colour as the effect collapses.
  static uint8_t depthCue(uint8_t comp, int32_t ir0, int32_t farComp);
};
