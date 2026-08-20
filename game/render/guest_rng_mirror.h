// game/render/guest_rng_mirror.h — GuestRngMirror: a read-only stand-in for the guest PRNG.
//
// THE PROBLEM IT SOLVES (claim C018, kanban #69). Several effect emitters take a jitter/dither from
// the guest PRNG FUN_8009A450, which READS AND WRITES the seed at 0x80105EE8. pc_render is a
// read-only overlay, so a native producer must not call it — advancing that seed is a guest-memory
// write and would break the SBS byte-exact invariant that is Job #1. Two ported-blocked halves need
// it: the area-14 sprite tail (0x801104D0, 34 calls) and the area-4 mesh cue.
//
// THE DESIGN. Snapshot the guest's seed ONCE PER LOGIC FRAME and advance a private copy for the
// producer's own draws. The guest's own body still runs underneath and advances the real seed; this
// mirror never touches it.
//
// WHY A PER-FRAME SNAPSHOT IS THE RIGHT ANSWER, not a compromise:
//   * It cannot desync permanently. Re-seeding every logic frame means any divergence lasts one
//     frame, so the effect can never drift into a visibly different regime.
//   * It is stable within a frame. Keying the snapshot on gpu.s_frame (the EffectLerp / MoteStreaks
//     idiom) means both fps60 presents of the same logic frame draw the SAME jitter — snapshotting
//     per pass instead would make the in-between frame disagree with the real one and the effect
//     would visibly crawl at 60fps.
//   * Bit-exactness with the guest's own draw sequence is NOT required and deliberately not
//     attempted. CLAUDE.md's rule is to match the observable RESULT, not the mechanism; whether our
//     dither picks the guest's exact numbers is unobservable, whereas WRITING the seed to get them
//     would be a real regression. Chasing the exact sequence would also be futile: the substrate
//     walk may run before or after this producer within a frame, so "the guest's value at this
//     point" is not even well defined from here.
//
// The step and the draw are the guest's own, read off gen_func_8009A450 rather than assumed:
//   seed = seed * 0x41C64E6D + 12345          (the classic LCG constants)
//   draw = (seed >> 16) & 0x7FFF              LOGICAL shift, masked to 15 bits -> [0, 32767]
// Note the draw is NOT a signed high half — it is unsigned and 15-bit, so every value is positive.
#pragma once
#include <cstdint>
struct Core;

class GuestRngMirror {
public:
  // Guest seed word. Read once per logic frame; never written.
  static constexpr uint32_t kSeedAddr = 0x80105EE8u;

  // Re-seed from guest RAM if this is a new logic frame, then return the next draw, exactly as
  // FUN_8009A450 computes it: (seed >> 16) & 0x7FFF, i.e. [0, 32767]. Safe to call any number of
  // times per frame; the sequence restarts from the guest value at each new frame.
  int32_t next(Core *c);

  // The raw 32-bit state after stepping, for a caller that wants its own masking.
  uint32_t nextRaw(Core *c);

private:
  void frameSync(Core *c);
  uint32_t mState = 0;
  int mFrame = -1;
};
