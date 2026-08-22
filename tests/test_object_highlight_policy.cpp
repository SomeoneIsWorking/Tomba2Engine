#include "object_highlight_policy.h"

#include <cstdio>

namespace {

int expectEqual(const char *name, int actual, int expected) {
  if (actual == expected) {
    return 0;
  }
  std::fprintf(stderr, "object_highlight: %s actual=%d expected=%d\n", name, actual, expected);
  return 1;
}

} // namespace

int main() {
  int failed = 0;

  auto active = ObjectHighlightPolicy::activation(0x40u, 123);
  failed += expectEqual("fixed selector enabled", active.enabled, true);
  failed += expectEqual("fixed selector scale", active.scaleInput, 0x50);
  active = ObjectHighlightPolicy::activation(0x80u, -24);
  failed += expectEqual("dynamic selector enabled", active.enabled, true);
  failed += expectEqual("dynamic selector scale", active.scaleInput, -24);
  active = ObjectHighlightPolicy::activation(0xC0u, -24);
  failed += expectEqual("fixed selector wins", active.scaleInput, 0x50);
  active = ObjectHighlightPolicy::activation(0x0Fu, 123);
  failed += expectEqual("ordinary mesh has no highlight", active.enabled, false);

  failed += expectEqual("positive lateral scale", ObjectHighlightPolicy::lateralScale(0x50), 40);
  failed += expectEqual("negative lateral scale wraps byte", ObjectHighlightPolicy::lateralScale(-24), 1012);

  auto cue = ObjectHighlightPolicy::cue(1u);
  failed += expectEqual("mode 1 cue", cue.amount, 2048);
  failed += expectEqual("mode 1 bias", cue.sortBias, -10);
  cue = ObjectHighlightPolicy::cue(2u);
  failed += expectEqual("mode 2 cue", cue.amount, 2048);
  cue = ObjectHighlightPolicy::cue(7u);
  failed += expectEqual("mode 7 cue", cue.amount, 1024);
  failed += expectEqual("mode 7 bias", cue.sortBias, -3);
  cue = ObjectHighlightPolicy::cue(0u);
  failed += expectEqual("ordinary mode cue", cue.amount, 0);
  failed += expectEqual("ordinary mode bias", cue.sortBias, -10);

  return failed == 0 ? 0 : 1;
}
