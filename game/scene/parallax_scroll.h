// The parallax backdrop's SCROLL DOMAIN: the two different modulus policies the title applies to a
// scroll offset, named apart so they cannot be mistaken for each other again.
//
// They ARE different, measured: over v in [-1024, 1024] at mod = 256 they disagree on 1,793 of
// 2,049 values. They were previously two undeclared copies in two files, each documented as a copy
// of the other, which is how the difference stayed invisible. Both are correct for their own caller;
// unifying them would change what the title draws.
//
//   guestReduce      — what ParallaxBg::step does to a freshly computed offset. The guest's
//                      over-then-rollback loop. IDENTITY on [0, mod); outside it, reduces to the
//                      nearest modulus-equivalent value one step OUT of range. It preserves the
//                      guest's representation rather than canonicalising it.
//   canonicalMod     — a true wrap into [0, mod).
//   shortestPathLerp — interpolates two offsets the short way around the wrap, canonicalising the
//                      result. Used only by the interpolated present.
//
// Whether the two can ever disagree ON SCREEN is a separate, open question: the backdrop drawer is
// exactly periodic in scrollX with period mod_x (= W*16, the same value the guest stamps), so an X
// representation difference is invisible, but it is NOT periodic in scrollY, because mod_y is
// (H*0x8E8)/0x90 rather than H*16. See docs/issues/0021.
//
// Pure arithmetic over plain integers: no Core, no guest read, no state. ParallaxBg owns where the
// moduli come from; this owns what they mean.
#pragma once
#include <cmath>
#include <cstdint>

namespace tomba::parallax {

// The guest's own reduction, transcribed from the instruction path's over-then-rollback loops.
// REQUIRES mod > 0. Returns `v` unchanged whenever 0 <= v < mod.
//
// This is NOT a wrap into [0, mod) and does not agree with `v % mod`: for v = -5, mod = 256 it
// returns -5, and for v = 600 it returns 344. That is deliberate — it is what the title computes,
// and ParallaxBg::step stores the result as a signed 16-bit field that later readers sign-extend.
inline int32_t guestReduce(int32_t v, int32_t mod) {
  if (v < 0) {
    v += mod;
    while (v < 0) {
      v += mod;
    }
    v -= mod;
  }
  if (mod <= v) {
    v -= mod;
    while (mod <= v) {
      v -= mod;
    }
    v += mod;
  }
  return v;
}

// A true wrap into [0, mod). REQUIRES mod > 0.
inline int32_t canonicalMod(int32_t v, int32_t mod) {
  const int32_t r = v % mod;
  return r < 0 ? r + mod : r;
}

// Interpolate `prev` -> `cur` at `t` along the SHORTEST signed path around a [0, mod) wrap, then
// canonicalise.
//
// A naive prev + (cur - prev) * t sweeps the long way whenever the pair straddles the boundary: at
// mod = 256, prev = 254, cur = 2 it walks 254 -> 128 -> 2 instead of 254 -> 0 -> 2. Working on the
// difference also makes this insensitive to WHICH modulus-equivalent representation either endpoint
// arrived in, which matters because guestReduce does not canonicalise its output.
//
// mod <= 0 means the caller has no wrap domain, so the plain lerp is returned unwrapped rather than
// handed to canonicalMod, whose contract it would violate.
inline int32_t shortestPathLerp(int32_t prev, int32_t cur, int32_t mod, float t) {
  if (mod <= 0) {
    return prev + (int32_t)std::lroundf((float)(cur - prev) * t);
  }
  int32_t diff = cur - prev;
  if (diff > mod / 2) {
    diff -= mod;
  }
  if (diff < -mod / 2) {
    diff += mod;
  }
  return canonicalMod(prev + (int32_t)std::lroundf((float)diff * t), mod);
}

} // namespace tomba::parallax
