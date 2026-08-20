// game/render/cube_text_banner_selftest.cpp — the acceptance test for the rule, not for the code.
//
// USER, 2026-08-04, on recovering an object's transform from PSX engine state: "never do this please
// NEVER, just leaving the effect as is is better than this." The observable consequence of breaking
// that rule, measured in kanban #71, was that the item banner's screen position moved WHEN THE CAMERA
// MOVED even though nothing in the game moved it. So the property this test asserts is exactly that:
//
//     CubeTextBanner's screen output does not change when the scene camera changes.
//
// It is a property of the DESIGN (the producer is view-space and never reads the camera), so it will
// keep holding as the producer grows — and it will fail loudly the moment someone reintroduces a
// camera term, which is the failure mode the rule exists to prevent.
//
// HOW IT CANNOT SILENTLY PASS. Three ways this test refuses to look green while testing nothing:
//   1. It asserts the producer EMITTED something (a nonzero prim count), and how many. A producer
//      that drew nothing would otherwise be trivially camera-invariant.
//   2. It runs a NEGATIVE CONTROL through the same comparator: the identical points projected the way
//      a camera-composed producer would project them (Rcam*p + Tcam) MUST differ across the same two
//      cameras. If the comparator could not tell the two cameras apart, the positive result would be
//      meaningless. The discriminator is therefore run against both classes, not reasoned about.
//   3. Both cameras are printed, so "the two cameras were accidentally equal" is visible.
//
// Selected by PSXPORT_SELFTEST=cubetext. Exit 0 = pass, 1 = fail. Needs MAIN.EXE, no disc, no GPU.
#include "core.h"
#include "cube_text_banner.h"
#include "game.h"
#include "game_ctx.h"
#include "render.h"
#include "render_queue.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <lucent/log.h>
#include <vector>

void load_exe(const char *path, Core *c);

namespace {

// Free guest RAM for the synthetic banner. Chosen well clear of MAIN.EXE's own image.
constexpr uint32_t NODE = 0x800B0000u;
constexpr uint32_t RECS = 0x800B0400u; // 8 records, 0x44 apart
constexpr uint32_t GEOMBLK = 0x800B0800u;
constexpr uint32_t TEXT = 0x800B0C00u;
constexpr uint32_t CAM_MTX = 0x1F8000F8u; // the scratchpad scene camera
constexpr uint32_t GTE_OFX = 24, GTE_OFY = 25, GTE_H = 26;

constexpr int kGlyphs = 6;
const char kText[] = "Ab Cd"; // includes a SPACE: a record with a plank but no letter

// Write a libgte MATRIX (5 CR-packed rotation words + 3 s32 translation) into guest memory.
void writeMatrix(Core *c, uint32_t at, const int16_t m[9], const int32_t t[3]) {
  c->mem_w32(at + 0, (uint16_t)m[0] | ((uint32_t)(uint16_t)m[1] << 16));
  c->mem_w32(at + 4, (uint16_t)m[2] | ((uint32_t)(uint16_t)m[3] << 16));
  c->mem_w32(at + 8, (uint16_t)m[4] | ((uint32_t)(uint16_t)m[5] << 16));
  c->mem_w32(at + 12, (uint16_t)m[6] | ((uint32_t)(uint16_t)m[7] << 16));
  c->mem_w32(at + 16, (uint16_t)m[8]);
  for (int i = 0; i < 3; i++) {
    c->mem_w32(at + 0x14u + (uint32_t)i * 4u, (uint32_t)t[i]);
  }
}

// Seed a banner the producer will accept: the behaviour pointer that IS its class identity, the two
// counts, the view-space position, and kGlyphs records each carrying a one-quad plank mesh.
void seedBanner(Core *c) {
  for (uint32_t a = NODE; a < NODE + 0x200u; a += 4) {
    c->mem_w32(a, 0);
  }
  c->mem_w32(NODE + 0x1Cu, CubeTextBanner::kBehCubeTextSpawn);
  c->mem_w8(NODE + 0x03u, 0); // variant 0 (not the "Clear" banner)
  c->mem_w8(NODE + 0x08u, kGlyphs);
  c->mem_w8(NODE + 0x09u, kGlyphs);
  c->mem_w16(NODE + 0x2Eu, 0);             // node X
  c->mem_w16(NODE + 0x32u, (uint16_t)-64); // node Y
  c->mem_w16(NODE + 0x36u, 358);           // node Z = H+8, the settled rise-in value
  for (int i = 0; i < 3; i++) {
    c->mem_w16(NODE + 0x54u + (uint32_t)i * 2u, 0); // node eulers
  }

  // One GT4 plank prim per record: counts word = (nGT4 << 16) | nGT3.
  for (uint32_t a = GEOMBLK; a < GEOMBLK + 0x100u; a += 4) {
    c->mem_w32(a, 0);
  }
  c->mem_w32(GEOMBLK + 0, 1u << 16);
  const uint32_t p = GEOMBLK + 16;
  c->mem_w32(p + 0, 0x3C808080u);                                                // rgb0 | op
  c->mem_w32(p + 4, 0x00808080u);                                                // rgb2
  c->mem_w32(p + 8, 0x2C5F0000u);                                                // uv0 | clut
  c->mem_w32(p + 12, 0x00050000u);                                               // uv1 | tpage
  c->mem_w32(p + 16, 0x00000000u);                                               // uv2 | uv3
  c->mem_w32(p + 20, ((uint32_t)(uint16_t)-6) | (6u << 16));                     // XY0
  c->mem_w32(p + 24, 0);                                                         // Z0 | Z1
  c->mem_w32(p + 28, 5u | (6u << 16));                                           // XY1
  c->mem_w32(p + 32, ((uint32_t)(uint16_t)-6) | ((uint32_t)(uint16_t)-8 << 16)); // XY2
  c->mem_w32(p + 36, 0);                                                         // Z2 | Z3
  c->mem_w32(p + 40, 5u | ((uint32_t)(uint16_t)-8 << 16));                       // XY3

  for (int i = 0; i < kGlyphs; i++) {
    const uint32_t rec = RECS + (uint32_t)i * 0x44u;
    for (uint32_t a = rec; a < rec + 0x44u; a += 4) {
      c->mem_w32(a, 0);
    }
    c->mem_w16(rec + 0x00u, (uint16_t)(int16_t)((i - kGlyphs / 2) * 12)); // layout X
    c->mem_w16(rec + 0x02u, (uint16_t)(int16_t)(-3 + i));                 // bob Y
    c->mem_w16(rec + 0x08u, (uint16_t)(int16_t)(i * 200));                // toss spin (nonzero!)
    c->mem_w32(rec + 0x40u, GEOMBLK);
    c->mem_w32(NODE + 0xC0u + (uint32_t)i * 4u, rec);
  }
  for (uint32_t i = 0; i < sizeof(kText); i++) {
    c->mem_w8(TEXT + i, (uint8_t)kText[i]);
  }
  // Point the node at a cube-text string-table entry (0x800A33C8, stride 12; the producer reads the
  // char* at +4) so the GLYPH half runs too — without this the test would only cover the planks.
  c->mem_w16(NODE + 0x60u, 1);
  c->mem_w32(0x800A33C8u + 12u + 4u, TEXT);
}

// Install a camera: the scratchpad MATRIX (what a factoring producer reads) AND the GTE control
// registers CR24-26 (OFX/OFY/H, which sceneCam reads). H stays fixed across the two cameras — it is
// the projection, not the camera pose, and changing it would confound the test.
void setCamera(Core *c, float yawTurns, int32_t tx, int32_t ty, int32_t tz) {
  const float a = yawTurns * 6.28318530718f;
  const int16_t s = (int16_t)std::lround(std::sin(a) * 4096.0), co = (int16_t)std::lround(std::cos(a) * 4096.0);
  const int16_t m[9] = {co, 0, s, 0, 4096, 0, (int16_t)-s, 0, co};
  const int32_t t[3] = {tx, ty, tz};
  writeMatrix(c, CAM_MTX, m, t);
  gte_write_ctrl(GTE_OFX, (uint32_t)(160 << 16));
  gte_write_ctrl(GTE_OFY, (uint32_t)(120 << 16));
  gte_write_ctrl(GTE_H, 350u);
}

struct Shot {
  std::vector<float> xy;
};

// Run the producer once and take every screen vertex it emitted, in emit order.
Shot shoot(Core *c) {
  RenderQueue &rq = c->game->rq;
  rq.reset();
  CubeTextBanner::render(c, NODE);
  Shot s;
  for (int i = 0; i < rq.n; i++) {
    for (int v = 0; v < rq.items[i].nv; v++) {
      s.xy.push_back(rq.items[i].xsf[v]);
      s.xy.push_back(rq.items[i].ysf[v]);
    }
  }
  return s;
}

// Largest |difference| between two shots, or -1 if they are not even the same shape (which is itself
// a failure — a producer that emits a different number of prims under a different camera is exactly
// as camera-dependent as one that moves them).
float maxDelta(const Shot &a, const Shot &b) {
  if (a.xy.size() != b.xy.size()) {
    return -1.0f;
  }
  float worst = 0;
  for (size_t i = 0; i < a.xy.size(); i++) {
    const float d = std::fabs(a.xy[i] - b.xy[i]);
    if (d > worst) {
      worst = d;
    }
  }
  return worst;
}

// THE NEGATIVE CONTROL. Project the banner's own points the way a CAMERA-COMPOSED producer would —
// view = Rcam*world + Tcam — under the same two cameras, through the same comparator. This must move.
// If it does not, the two cameras are not actually different and the positive result above proves
// nothing.
Shot shootAsIfCameraComposed(Core *c) {
  constexpr float FX = 1.0f / 4096.0f;
  float camR[3][3], camT[3];
  Render::readSceneViewMatrix(c, camR, camT); // CAM_MTX layout lives in ONE decoder (render.h)
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      camR[i][j] *= FX; // 1.3.12 -> unit scale
    }
  }

  Shot s;
  for (int i = 0; i < kGlyphs; i++) {
    const uint32_t rec = RECS + (uint32_t)i * 0x44u;
    const float p[3] = {(float)c->mem_r16s(rec + 0), (float)c->mem_r16s(rec + 2) - 64.0f, 358.0f};
    float v[3];
    for (int k = 0; k < 3; k++) {
      v[k] = camR[k][0] * p[0] + camR[k][1] * p[1] + camR[k][2] * p[2] + camT[k];
    }
    const float pz = v[2] > 175.0f ? v[2] : 175.0f;
    s.xy.push_back(160.0f + v[0] * 350.0f / pz);
    s.xy.push_back(120.0f + v[1] * 350.0f / pz);
  }
  return s;
}

} // namespace

int run_cubetext_selftest(const char *path) {
  Game *game = new Game();
  Core *c = &game->core;
  load_exe(path, c);
  seedBanner(c);

  // Two genuinely different cameras: a 1/8-turn yaw apart and 900 units apart in view translation —
  // far more than the 383 units of pan that produced kanban #71's 1.53 px of vibration.
  setCamera(c, 0.000f, 100, -50, 4000);
  const Shot camA = shoot(c);
  const Shot ctrlA = shootAsIfCameraComposed(c);
  setCamera(c, 0.125f, 1000, 260, 4700);
  const Shot camB = shoot(c);
  const Shot ctrlB = shootAsIfCameraComposed(c);

  const size_t verts = camA.xy.size() / 2;
  lucent::info(
      "cubetesttest", "camera A -> {} prims / {} verts; camera B -> {} verts", game->rq.n, verts, camB.xy.size() / 2);

  // (1) The producer must have DRAWN something, or camera-invariance is vacuous. 6 records x 1 plank
  //     quad = 6, plus 4 letters ("Ab Cd" has 5 chars, one of them a space, and the trailing NUL ends
  //     the loop) = 10 quads, 40 verts.
  if (verts == 0) {
    lucent::error("cubetesttest", "the producer emitted NOTHING — this test verified nothing");
    return 1;
  }
  if (verts != 40) {
    lucent::error("cubetesttest", "expected 40 verts (6 planks + 4 letters, one record is a SPACE), got {}", verts);
    return 1;
  }

  // (2) The negative control must move, or the comparator cannot see a camera change at all.
  const float ctrl = maxDelta(ctrlA, ctrlB);
  if (ctrl <= 1.0f) {
    lucent::error("cubetesttest",
                  "NEGATIVE CONTROL FAILED: a camera-composed projection of the same points moved "
                  "only {:.4f} px between the two cameras. The cameras are not different enough for "
                  "this test to mean anything.",
                  ctrl);
    return 1;
  }
  lucent::info("cubetesttest", "negative control: a camera-composed projection moves {:.1f} px", ctrl);

  // (3) The real assertion. EXACTLY zero — not "small". The producer never reads the camera, so any
  //     nonzero value means a camera term got in, and the size of it is not the point.
  const float delta = maxDelta(camA, camB);
  if (delta != 0.0f) {
    lucent::error("cubetesttest",
                  "FAIL: the banner moved {:.4f} px when only the CAMERA changed. Nothing in the game "
                  "moved it — a camera term has re-entered the producer.",
                  delta < 0 ? -1.0f : delta);
    return 1;
  }
  lucent::info("cubetesttest",
               "PASS: {} verts, byte-identical across two cameras {:.1f} px apart by the control's "
               "own measure.",
               verts,
               ctrl);
  return 0;
}
