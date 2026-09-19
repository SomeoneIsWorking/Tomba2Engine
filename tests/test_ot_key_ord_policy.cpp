// The inverse of the guest's OT key compression must be STRICTLY increasing in the key, because the
// framework's ordering resolver aborts when two buckets land on one ord. The case that matters is
// the run of keys the compression cannot produce (1792..1919), which the previous inverse collapsed
// onto key 1920's otz -- measured as a deterministic crash of Tomba! 2 at frame 2700.
#include "render/ot_key_ord_policy.h"

#include <cstdio>
#include <set>

namespace {

int failures = 0;

void check(bool condition, const char *what) {
  if (!condition) {
    std::printf("  FAIL %s\n", what);
    ++failures;
  }
}

} // namespace

int main() {
  using namespace tomba2::ot_key_ord;

  // 1. The exact answers are unchanged wherever the compression reaches the key. This is the half
  //    that must NOT move: the interpolation is only allowed to fill gaps.
  int reachable = 0;
  for (int32_t key = 4; key < 2048; ++key) {
    const int32_t low = firstOtzAtOrAbove(key);
    if (forward(low) != key) {
      continue;
    }
    ++reachable;
    check(otzForKey(key) == (double)low, "an exactly reachable key kept its exact inverse");
  }
  check(reachable == 1916, "1916 of the admitted keys are reachable");

  // 2. The gap is real and is the one measured: exactly 128 admitted keys, contiguous, 1792..1919.
  int gaps = 0;
  int32_t firstGap = -1;
  int32_t lastGap = -1;
  for (int32_t key = 4; key < 2048; ++key) {
    if (forward(firstOtzAtOrAbove(key)) != key) {
      ++gaps;
      if (firstGap < 0) {
        firstGap = key;
      }
      lastGap = key;
    }
  }
  check(gaps == 128, "128 admitted keys are unreachable");
  check(firstGap == 1792 && lastGap == 1919, "the unreachable run is 1792..1919");

  // 3. THE POINT: strictly increasing across every admitted key, gaps included. The old inverse
  //    failed this at 1792..1920 and that is what aborted the product.
  double previous = -1.0;
  for (int32_t key = 4; key < 2048; ++key) {
    const double value = otzForKey(key);
    check(value > previous, "otzForKey is strictly increasing");
    previous = value;
  }

  // 4. The negative that would make check 3 vacuous: the OLD inverse must FAIL it. If the plain
  //    search were already strictly increasing there was never a bug, and this test would be
  //    asserting a property nothing ever violated.
  std::set<int32_t> collapsed;
  for (int32_t key = 1792; key <= 1920; ++key) {
    collapsed.insert(firstOtzAtOrAbove(key));
  }
  check(collapsed.size() == 1, "the plain search really does collapse 1792..1920 onto ONE otz");
  check(*collapsed.begin() == 3072, "and that one otz is 3072, the value the crash reported");

  if (failures != 0) {
    std::printf("test_ot_key_ord_policy: %d check(s) FAILED\n", failures);
    return 1;
  }
  std::printf("test_ot_key_ord_policy: PASS (exact inverse preserved on all %d reachable keys; "
              "128-key gap 1792..1919 measured; strictly increasing across all 2044 admitted keys; "
              "and the old plain search is shown to collapse that gap onto otz 3072)\n",
              reachable);
  return 0;
}
