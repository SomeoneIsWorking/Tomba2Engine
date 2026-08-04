// game/render/cube_text_banner.h — the native pc_render producer for the CUBE-TEXT BANNER
// (the item/quest announcement strip: "A Red Treasure Chest", "Find Tabby!", "Clear").
//
// This replaces two deleted TAPS. Until 2026-08-04 the banner's picture came from reading the
// per-glyph libgte MATRIX the guest had already composed (cmd+0x18 / sub+0x18), un-composing the
// scene camera out of it (wq_factor_world) and letting the display pass re-apply the camera. That
// is banned outright — USER, 2026-08-04: "never do this please NEVER, just leaving the effect as is
// is better than this" — and it was the measured cause of kanban #71's vibration: cam^T-then-cam is
// identity only in exact arithmetic, the camera is s16 fixed point, so the residue was a FUNCTION OF
// THE CAMERA (camera still: mean |dX| 0.13 px; camera panning: 1.53 px, 12/12 sign alternations).
//
// This producer reads the banner's OWN state and never touches the composed matrix, the GTE, the
// packet pool or the OT. And the decisive property: **the camera does not enter its arithmetic at
// all**. The cube-text node is a VIEW-SPACE billboard — its transform IS the modelview — so the
// producer projects with ofx/ofy/H alone. A camera-dependent residue is not reduced here, it is
// structurally impossible.
#pragma once
#include <cstdint>
struct Core;

class CubeTextBanner {
public:
  // The behaviour pointer stored at node+0x1C by CubeTextLedger::spawnPopup. This IS the class
  // identity — the producer selects the node by what it is, not by a tag anyone stamped on it.
  static constexpr uint32_t kBehCubeTextSpawn = 0x8003AD48u;

  // Draw one node's banner, if that node is a cube-text banner. Called from the native object walk
  // (render_walk.cpp) for every pre-composed-matrix node; self-filters on the behaviour pointer, so
  // a non-banner node of the same render class produces nothing rather than a guessed-transform mesh.
  // READ-ONLY: guest memory is only read, no c->r[] is touched, nothing runs on the oracle leg.
  static void render(Core* c, uint32_t node);
};
