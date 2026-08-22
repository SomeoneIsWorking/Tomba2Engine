#include "mesh_quads.h"
#include "prop_quad.h"

#include <cstdio>

namespace {

int expectEqual(const char *name, int actual, int expected) {
  if (actual == expected) {
    return 0;
  }
  std::fprintf(stderr, "prop_quad: %s actual=%d expected=%d\n", name, actual, expected);
  return 1;
}

} // namespace

int main() {
  int failed = 0;
  failed += expectEqual("ordinary scale", PropQuadRecipe::columnScale(10u), 400);
  failed += expectEqual("scale byte wraps before widening", PropQuadRecipe::columnScale(26u), 16);

  failed += expectEqual("kind 1 keeps record tpage", PropQuadRecipe::tpageOverride(1u, 3u), -1);
  failed += expectEqual("kind 3 keeps record tpage", PropQuadRecipe::tpageOverride(3u, 9u), -1);
  failed += expectEqual("subkind 3 forces tpage", PropQuadRecipe::tpageOverride(0u, 3u), 46);
  failed += expectEqual("subkind 5 forces tpage", PropQuadRecipe::tpageOverride(2u, 5u), 46);
  failed += expectEqual("subkind 9 forces tpage", PropQuadRecipe::tpageOverride(0u, 9u), 46);
  failed += expectEqual("subkind 10 forces tpage", PropQuadRecipe::tpageOverride(2u, 10u), 46);
  failed += expectEqual("subkind 6 keeps record tpage", PropQuadRecipe::tpageOverride(0u, 6u), -1);
  failed += expectEqual("subkind 11 keeps record tpage", PropQuadRecipe::tpageOverride(0u, 11u), -1);

  failed += expectEqual("positive fog delta", MeshQuads::fogShade(120u, 40, 10), 90);
  failed += expectEqual("negative fog delta is ignored", MeshQuads::fogShade(120u, -10, 10), 120);
  failed += expectEqual("fog clamps low", MeshQuads::fogShade(12u, 127, -100), 0);
  return failed == 0 ? 0 : 1;
}
