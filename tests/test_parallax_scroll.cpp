// The parallax scroll domain's two modulus policies, pinned APART.
//
// They were once two copies in two files, each commented as a copy of the other, and they are not
// the same function. This test exists so that merging them fails loudly instead of silently moving
// the backdrop: guestReduce is what the title computes on a real frame, shortestPathLerp is what the
// interpolated present computes, and the difference between them is the thing under test.
#include "parallax_scroll.h"
#include "testutil.h"

using tomba::parallax::canonicalMod;
using tomba::parallax::guestReduce;
using tomba::parallax::shortestPathLerp;

// ---- guestReduce: identity in range, NOT a canonical wrap outside it ---------------------------

static void test_guest_reduce_is_the_identity_inside_the_range(void) {
  for (int32_t v = 0; v < 256; v++) {
    CHECK_EQ(guestReduce(v, 256), v);
  }
}

static void test_guest_reduce_is_not_a_canonical_wrap(void) {
  // The exact values the old docstring claimed were equal. They are not.
  CHECK_EQ(guestReduce(-5, 256), -5);
  CHECK_EQ(canonicalMod(-5, 256), 251);
  CHECK_EQ(guestReduce(600, 256), 344);
  CHECK_EQ(canonicalMod(600, 256), 88);
  CHECK_EQ(guestReduce(-513, 256), -1);
  CHECK_EQ(canonicalMod(-513, 256), 255);
}

static void test_guest_reduce_lands_within_one_modulus_of_the_range(void) {
  // What it DOES guarantee: the result is modulus-equivalent to the input and no further than one
  // modulus outside [0, mod). That is the property the signed-16-bit store depends on.
  const int32_t mod = 126; // the seaside's Y modulus shape: (H*0x8E8)/0x90, not H*16
  int32_t outside = 0;
  for (int32_t v = -600; v <= 600; v++) {
    const int32_t r = guestReduce(v, mod);
    CHECK_EQ((r - v) % mod, 0);
    CHECK(r >= -mod && r < 2 * mod);
    if (r < 0 || r >= mod) {
      outside++;
    }
  }
  // and it really does leave the range often — otherwise the check above proves nothing
  CHECK(outside > 400);
}

// ---- canonicalMod -----------------------------------------------------------------------------

static void test_canonical_mod_wraps_both_directions(void) {
  CHECK_EQ(canonicalMod(0, 256), 0);
  CHECK_EQ(canonicalMod(255, 256), 255);
  CHECK_EQ(canonicalMod(256, 256), 0);
  CHECK_EQ(canonicalMod(-1, 256), 255);
  CHECK_EQ(canonicalMod(-256, 256), 0);
  CHECK_EQ(canonicalMod(700, 256), 188);
}

// ---- shortestPathLerp -------------------------------------------------------------------------

static void test_the_lerp_reproduces_its_endpoints(void) {
  CHECK_EQ(shortestPathLerp(40, 90, 256, 0.0f), 40);
  CHECK_EQ(shortestPathLerp(40, 90, 256, 1.0f), 90);
  CHECK_EQ(shortestPathLerp(40, 90, 256, 0.5f), 65);
}

static void test_the_lerp_takes_the_short_way_across_the_wrap(void) {
  // 254 -> 2 at mod 256 is 4 forward, not 252 backward. Halfway is 0, not 128.
  CHECK_EQ(shortestPathLerp(254, 2, 256, 0.5f), 0);
  CHECK_EQ(shortestPathLerp(2, 254, 256, 0.5f), 0);
  // and the long way is what a naive lerp would give, so the two are distinguishable here
  CHECK(shortestPathLerp(254, 2, 256, 0.5f) != 128);
}

static void test_the_lerp_result_is_always_canonical(void) {
  const int32_t mod = 126;
  for (int32_t prev = 0; prev < mod; prev += 7) {
    for (int32_t cur = 0; cur < mod; cur += 5) {
      for (int step = 0; step <= 4; step++) {
        const int32_t v = shortestPathLerp(prev, cur, mod, (float)step * 0.25f);
        CHECK(v >= 0 && v < mod);
      }
    }
  }
}

static void test_the_lerp_ignores_which_representation_its_endpoints_arrived_in(void) {
  // guestReduce does not canonicalise, so an endpoint can arrive one modulus out of range. The lerp
  // works on the difference, so it must not care. This is what keeps the interpolated backdrop tied
  // to the real frames even though the two policies differ.
  const int32_t mod = 256;
  CHECK_EQ(shortestPathLerp(-2, 10, mod, 0.5f), shortestPathLerp(254, 10, mod, 0.5f));
  CHECK_EQ(shortestPathLerp(40, 300, mod, 0.5f), shortestPathLerp(40, 44, mod, 0.5f));
  CHECK_EQ(shortestPathLerp(-5, -1, mod, 0.5f), shortestPathLerp(251, 255, mod, 0.5f));
}

static void test_a_missing_modulus_is_a_plain_lerp(void) {
  // mod <= 0 means there is no wrap domain; canonicalMod's contract would be violated, so the
  // unwrapped lerp is returned instead. Negative results are expected and must survive.
  CHECK_EQ(shortestPathLerp(10, 20, 0, 0.5f), 15);
  CHECK_EQ(shortestPathLerp(10, -30, 0, 0.5f), -10);
  CHECK_EQ(shortestPathLerp(10, 20, -4, 1.0f), 20);
}

int main(void) {
  RUN(guest_reduce_is_the_identity_inside_the_range);
  RUN(guest_reduce_is_not_a_canonical_wrap);
  RUN(guest_reduce_lands_within_one_modulus_of_the_range);
  RUN(canonical_mod_wraps_both_directions);
  RUN(the_lerp_reproduces_its_endpoints);
  RUN(the_lerp_takes_the_short_way_across_the_wrap);
  RUN(the_lerp_result_is_always_canonical);
  RUN(the_lerp_ignores_which_representation_its_endpoints_arrived_in);
  RUN(a_missing_modulus_is_a_plain_lerp);
  return pt_summary();
}
