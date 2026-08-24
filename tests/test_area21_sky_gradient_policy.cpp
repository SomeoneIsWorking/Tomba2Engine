#include "area21_sky_gradient_policy.h"

#include <cstdio>

namespace {

int expectEqual(const char *name, int actual, int expected) {
  if (actual == expected) {
    return 0;
  }
  std::fprintf(stderr, "area21_sky_gradient: %s actual=%d expected=%d\n", name, actual, expected);
  return 1;
}

} // namespace

int main() {
  int failed = 0;
  failed += expectEqual("reached branch active", Area21SkyGradientPolicy::active(0u, 21u, 1u, 1u), true);
  failed += expectEqual("dispatch gate rejects", Area21SkyGradientPolicy::active(1u, 21u, 1u, 1u), false);
  failed += expectEqual("other state rejects", Area21SkyGradientPolicy::active(0u, 20u, 1u, 1u), false);
  failed += expectEqual("tilemap variant rejects", Area21SkyGradientPolicy::active(0u, 21u, 0u, 1u), false);
  failed += expectEqual("late phase rejects", Area21SkyGradientPolicy::active(0u, 21u, 1u, 4u), false);

  failed += expectEqual("zero-pitch origin", Area21SkyGradientPolicy::originY(0), 120);
  failed += expectEqual("observed negative-pitch origin", Area21SkyGradientPolicy::originY(-175), 217);
  const auto bands = Area21SkyGradientPolicy::bands(-175);
  constexpr int expectedTop[4] = {-223, 157, 217, 277};
  constexpr int expectedBottom[4] = {157, 217, 277, 697};
  constexpr Area21SkyGradientPolicy::Color expectedTopColor[4] = {
      {6, 6, 172}, {6, 6, 172}, {152, 152, 234}, {0, 0, 57}};
  constexpr Area21SkyGradientPolicy::Color expectedBottomColor[4] = {
      {6, 6, 172}, {152, 152, 234}, {0, 0, 57}, {0, 0, 57}};
  for (int i = 0; i < 4; ++i) {
    failed += expectEqual("band top", bands[i].top, expectedTop[i]);
    failed += expectEqual("band bottom", bands[i].bottom, expectedBottom[i]);
    failed += expectEqual("band top red", bands[i].topColor.r, expectedTopColor[i].r);
    failed += expectEqual("band top green", bands[i].topColor.g, expectedTopColor[i].g);
    failed += expectEqual("band top blue", bands[i].topColor.b, expectedTopColor[i].b);
    failed += expectEqual("band bottom red", bands[i].bottomColor.r, expectedBottomColor[i].r);
    failed += expectEqual("band bottom green", bands[i].bottomColor.g, expectedBottomColor[i].g);
    failed += expectEqual("band bottom blue", bands[i].bottomColor.b, expectedBottomColor[i].b);
  }
  return failed == 0 ? 0 : 1;
}
