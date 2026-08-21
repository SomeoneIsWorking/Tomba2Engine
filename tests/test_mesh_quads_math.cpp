#include "mesh_quads.h"

#include <cstdio>

int main() {
  const int32_t rotation[3][3] = {{4096, -2048, 0}, {1024, 4096, -512}, {0, 2048, 4096}};
  const int32_t identity[3][3] = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}};
  // FUN_8002B3A4 reads three 0x10 bytes and shifts them left by two before Math::matColScale.
  const int32_t columnScale[3] = {64, 64, 64};
  float actual[3][3] = {};
  MeshQuads::composeScaled(rotation, identity, columnScale, actual);

  int failed = 0;
  for (int row = 0; row < 3; ++row) {
    for (int column = 0; column < 3; ++column) {
      const float expected = static_cast<float>((rotation[row][column] * columnScale[column]) >> 12);
      if (actual[row][column] == expected) {
        continue;
      }
      std::fprintf(stderr,
                   "mesh_quads_math: scaled ring matrix mismatch at [%d][%d] (actual=%.3f expected=%.3f)\n",
                   row,
                   column,
                   static_cast<double>(actual[row][column]),
                   static_cast<double>(expected));
      ++failed;
    }
  }
  return failed == 0 ? 0 : 1;
}
