#pragma once

#include <cstdint>

// The inverse of Tomba! 2's ordering-table key compression, as a CONTINUOUS function of the key.
//
// The guest compresses a 16-bit `otz` into a key with `b = otz >> 10; k = (otz >> b) + (b << 9)`.
// That map is nondecreasing but NOT onto: within the key range the guest's own range test admits
// (4 <= k < 2048) it reaches 1,916 keys and skips 128 of them -- one contiguous run, keys
// 1792..1919, between band 2's top key (1791) and band 3's bottom key (1920). A key lands in that
// run when the OT-base shift moves it there, which the submitter applies AFTER the guest's range
// test.
//
// A plain "smallest otz whose forward map reaches this key" search sends every one of those 128
// keys, and key 1920, to otz 3072 -- one value, so one ord. The framework requires the key->ord map
// to be STRICTLY monotone across buckets and aborts when it is not, which is right: two buckets at
// one depth is an ordering the resolver cannot carry out. Measured 2026-09-19, Tomba! 2 died
// deterministically at frame 2700 on both aspect settings with
//
//     FATAL: OT key->ord is not strictly monotone at key 1920: ord 0.011602190, nearer band 0.011602190
//
// So the inverse is continuous instead. Where the compression reaches a key the answer is exact and
// unchanged; across a run it skips, the otz is interpolated between the neighbouring reachable keys'
// own inverses. That is the faithful statement: the guest's compression genuinely carries no
// distance for a key it cannot produce, but it does carry ORDER, and order is all the resolver
// needs. Interpolating recovers the order without inventing a distance, and it is not a tuning
// constant -- the endpoints are the two neighbouring keys' exact inverses.
namespace tomba2::ot_key_ord {

// The guest's own compression, `0x80080000`'s literal arithmetic.
inline constexpr int32_t forward(int32_t otz) {
  const int32_t band = otz >> 10;
  return (otz >> (band & 31)) + (band << 9);
}

// The smallest otz whose forward map reaches `key`, or 65535 when none does.
inline constexpr int32_t firstOtzAtOrAbove(int32_t key) {
  int32_t low = 0;
  int32_t high = 65535;
  while (low < high) {
    const int32_t mid = (low + high) >> 1;
    if (forward(mid) < key) {
      low = mid + 1;
    } else {
      high = mid;
    }
  }
  return low;
}

// The continuous inverse: exact where the compression reaches `key`, interpolated across the runs
// it skips. Strictly increasing in `key` wherever the exact inverse is nondecreasing.
inline double otzForKey(int32_t key) {
  const int32_t low = firstOtzAtOrAbove(key);
  if (forward(low) == key || low <= 0) {
    return (double)low;
  }
  // `key` is inside a skipped run. Its neighbours are the largest key the compression reaches below
  // it and the one it reaches at `low`; both are placed at their OWN exact inverses so the
  // interpolation agrees with them at the endpoints.
  const int32_t previousKey = forward(low - 1);
  const int32_t nextKey = forward(low);
  if (nextKey <= previousKey) {
    return (double)low;
  }
  const double previousOtz = (double)firstOtzAtOrAbove(previousKey);
  const double span = (double)(nextKey - previousKey);
  return previousOtz + ((double)low - previousOtz) * (double)(key - previousKey) / span;
}

} // namespace tomba2::ot_key_ord
