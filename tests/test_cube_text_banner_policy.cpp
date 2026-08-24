#include "cube_text_banner.h"

#include <cstdio>

namespace {

int expectEqual(const char *name, bool actual, bool expected) {
  if (actual == expected) {
    return 0;
  }
  std::fprintf(stderr, "cube_text_banner: %s actual=%d expected=%d\n", name, actual, expected);
  return 1;
}

} // namespace

int main() {
  int failed = 0;
  failed += expectEqual("presentation builds picture", CubeTextBanner::pictureBuildAllowed(false, false), true);
  failed += expectEqual("oracle produces no picture", CubeTextBanner::pictureBuildAllowed(true, false), false);
  failed += expectEqual("capture-only produces no picture", CubeTextBanner::pictureBuildAllowed(false, true), false);
  failed += expectEqual("oracle capture produces no picture", CubeTextBanner::pictureBuildAllowed(true, true), false);
  return failed == 0 ? 0 : 1;
}
