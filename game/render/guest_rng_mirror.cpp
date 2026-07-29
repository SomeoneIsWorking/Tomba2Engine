// game/render/guest_rng_mirror.cpp — GuestRngMirror (see guest_rng_mirror.h for why it exists).
#include "guest_rng_mirror.h"
#include "core.h"
#include "game.h"

namespace {
// gen_func_8009A450's own constants: multiplier 0x41C60000 | 20077, increment 12345.
constexpr uint32_t kMul = 0x41C64E6Du;
constexpr uint32_t kInc = 12345u;
}  // namespace

// One re-seed per LOGIC frame. Both fps60 presents share an s_frame, so keying on it makes the two
// passes of the same frame draw the same sequence — per-pass re-seeding would make the in-between
// frame disagree with the real one and the jitter would visibly crawl at 60fps.
void GuestRngMirror::frameSync(Core* c) {
  const int frame = c->game->gpu.s_frame;
  if (mFrame == frame) return;
  mFrame = frame;
  mState = c->mem_r32(kSeedAddr);   // READ ONLY — the guest's own body owns this word
}

uint32_t GuestRngMirror::nextRaw(Core* c) {
  frameSync(c);
  mState = mState * kMul + kInc;
  return mState;
}

int32_t GuestRngMirror::next(Core* c) {
  return (int32_t)((nextRaw(c) >> 16) & 0x7FFFu);
}
