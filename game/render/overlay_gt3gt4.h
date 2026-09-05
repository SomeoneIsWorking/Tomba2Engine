// game/render/overlay_gt3gt4.h — PC-native bodies for the A00-overlay GT3/GT4 render-packet
// emitter cluster (FUN_80146478/801465EC/801467BC). See overlay_gt3gt4.cpp for the full RE trace
// and wiring rationale.
//
// This is NOT pc_render (the read-only host-overlay picture pass). This is the SUBSTRATE's own
// GTE + OT + GP0 packet writer — it executes underneath on BOTH SBS cores and its guest-memory
// writes (packet-pool bump-allocator stores, OT bucket link) are part of the byte-exact faithful
// state. The native bodies below reproduce the guest MIPS 1:1 (same GTE ops, same guest
// writes, same field layout) — CLAUDE.md's "faithful substrate mirror" carve-out, distinct from
// the render-underneath / pc_render no-guest-write rule that governs game/render/submit.cpp's
// Render::submitPolyGt3Native / submitPolyGt4Native (the MAIN engine's GT3/GT4 path, which is
// deliberately NOT GTE-driven and NOT guest-writing).
#pragma once
struct Core;
class Game;

class OverlayGt3Gt4 {
public:
  // FUN_80146478(block=a0, ot_base=a1) -> first byte past the block's records, in v0.
  // The FIELD SUBMITTER's block dispatcher and the busiest single substrate-dispatch target in the
  // game (127,275 typed runtime address dispatch hits over 6000 frames of the seesaw-weight replay, 4x the runner-up).
  static void submitBlock(Core *c);

  static void gt3(Core *c); // FUN_801465EC(rec=a0, ot_base=a1, count=a2) -> advanced rec ptr in v0
  static void gt4(Core *c); // FUN_801467BC(rec=a0, ot_base=a1, count=a2) -> advanced rec ptr in v0

  // All three addresses go into the ONE process-global registry via tomba::native::declareOverride ->
  // tomba::native::declareOverride, which carries each address's { native, gen } pair and makes the single
  // oracle-leg decision. Passing the ov_a00 setter means the shared thunk also lands in that
  // overlay's own image-qualified runtime dispatcher table, so direct guest calls are intercepted alongside
  // typed runtime address dispatch ones — which matters here because the two leaves are reached by direct C call
  // (overlay guest 0x801465EC/801467BC(c)), never by typed runtime address dispatch. That covers both of their call
  // sites uniformly: this cluster's own dispatcher, and a duplicate tail-shared copy of the same
  // call sequence the recorded binary evidence's CFG discovery folded into FUN_80147FC4.
  //
  // (An older version of this comment claimed the leaves were wired via a RAW A00 tomba::native::declareOverride
  // "NOT the process-global registry". That described a wiring the file no longer had — the raw
  // form is precisely what reintroduces the fake-0-diff-on-core-B bug, and only PlatformHle may
  // use it.)
  static void registerOverrides(Game *game);
};
