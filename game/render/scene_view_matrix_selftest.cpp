// game/render/scene_view_matrix_selftest.cpp — PSXPORT_SELFTEST=sceneview
//
// Pins Render::readSceneViewMatrix as the exact INVERSE of Render::projActiveCr's packing, BY CODE.
//
// Why this exists: the scratchpad view-matrix decode used to be hand-written in three places (the
// framework's Fps60::sceneCam, the fps60ReadSceneCam game hook, and the cube-text selftest's negative
// control). When the decode moved behind the game seam, one copy picked R22 out of the HIGH halfword of
// the fifth word instead of the low one. Nothing compared the copies, so the whole native projection
// path ran on a corrupt view-Z row — the world stopped rendering and the camera read as broken. There is
// now ONE decoder; this test is what keeps it honest against the packer it must mirror.
//
// The test never hand-writes the halfword layout: it PACKS through projActiveCr and UNPACKS through
// readSceneViewMatrix, so a change to either side that the other does not follow fails here.
#include "core.h"
#include "game.h"
#include "game_ctx.h"
#include "projection.h"
#include "render.h"
#include <cmath>
#include <lucent/log.h>

namespace {

constexpr uint32_t kSceneViewMatrix = 0x1F8000F8u;

// Every element distinct and non-zero, so a swapped, duplicated or zeroed pick is visible. R22 in
// particular must be non-zero: the packer leaves the high half of its word clear, so the historical
// `>>16` misread returns 0 and would hide behind a zero expectation.
void makeXform(EObjXform *x) {
  const float R[3][3] = {{4096, -300, 700}, {150, 3900, -820}, {-640, 510, 3300}};
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      x->R[i][j] = R[i][j];
    }
  }
  x->T[0] = 1234;
  x->T[1] = -5678;
  x->T[2] = 90123;
  x->ofx = 160;
  x->ofy = 120;
  x->H = 350;
}

// Lay the packed control-register words down where the guest's libgte records them. cr[0..4] are the
// rotation halfword pairs, cr[5..7] the translation — the same block readSceneViewMatrix reads.
void writePacked(Core *c, const uint32_t cr[11]) {
  for (int i = 0; i < 5; i++) {
    c->mem_w32(kSceneViewMatrix + (uint32_t)i * 4u, cr[i]);
  }
  for (int i = 0; i < 3; i++) {
    c->mem_w32(kSceneViewMatrix + 0x14u + (uint32_t)i * 4u, cr[5 + i]);
  }
}

} // namespace

int run_sceneview_selftest(const char * /*exePath*/) {
  // No EXE needed: this exercises a memory layout, not game code. A Game gives us a Core with RAM.
  Game *game = new Game();
  Core *c = &game->core;

  EObjXform x;
  makeXform(&x);
  rend(c)->projSetActive(&x);
  uint32_t cr[11];
  rend(c)->projActiveCr(cr);
  rend(c)->projClearActive();
  writePacked(c, cr);

  float R[3][3], T[3];
  Render::readSceneViewMatrix(c, R, T);

  int bad = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (std::fabs(R[i][j] - x.R[i][j]) > 0.5f) {
        lucent::error("sceneview",
                      "R[{}][{}] round-tripped to {:.1f}, packed from {:.1f} — the decoder "
                      "and projActiveCr disagree about this halfword",
                      i,
                      j,
                      R[i][j],
                      x.R[i][j]);
        bad++;
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    if (std::fabs(T[i] - x.T[i]) > 0.5f) {
      lucent::error("sceneview", "T[{}] round-tripped to {:.1f}, packed from {:.1f}", i, T[i], x.T[i]);
      bad++;
    }
  }

  // THE NEGATIVE CONTROL. Take R22 from the high halfword — the exact historical misread — and prove
  // this comparison would have caught it. Without this the test could be passing because both sides
  // are equally wrong, or because the expectation is vacuous.
  const uint32_t w4 = c->mem_r32(kSceneViewMatrix + 16);
  const float wrongR22 = (float)(int16_t)(w4 >> 16);
  if (std::fabs(wrongR22 - x.R[2][2]) <= 0.5f) {
    lucent::error("sceneview",
                  "NEGATIVE CONTROL FAILED: the wrong halfword pick for R22 yields {:.1f}, "
                  "which equals the correct {:.1f}. This test cannot distinguish the bug it exists for — "
                  "choose an R22 whose two halfwords differ.",
                  wrongR22,
                  x.R[2][2]);
    bad++;
  }

  if (bad) {
    lucent::error("sceneview", "FAILED: {} mismatch(es) between projActiveCr and readSceneViewMatrix", bad);
    return 1;
  }
  lucent::info("sceneview",
               "OK: 9 rotation + 3 translation elements round-trip pack->unpack exactly; "
               "the wrong-halfword R22 read ({:.1f}) is correctly rejected against {:.1f}",
               wrongR22,
               x.R[2][2]);
  return 0;
}
