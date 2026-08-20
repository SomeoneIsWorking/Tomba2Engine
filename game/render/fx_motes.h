// game/render/fx_motes.h — MoteStreaks: the one-frame-behind SCREEN-POSITION shadow the area-8
// motion-streak producer draws from (guest FUN_80116904, see fx_motes.cpp).
//
// WHY THIS EXISTS AT ALL. The effect is inherently one-frame-differential: each mote is drawn as a
// streak from its CURRENT screen position back past its PREVIOUS one. The guest keeps that history in
// GUEST memory (a 32-entry {s16 x, s16 y} array at 0x801485E8, plus the previous cube base at
// node+0x48/0x4A/0x4C). pc_render must not write guest memory, and READING the guest's copy is not
// safe either — whether it holds this frame's or last frame's values depends on whether the substrate
// render walk has already run, which is not a property this producer controls. So the producer keeps
// its OWN host-side shadow. Host memory only; nothing here reads or writes anything the guest sees.
//
// The rotation idiom is EffectLerp's, deliberately: one rotation per LOGIC frame keyed on
// gpu.s_frame, because both fps60 presents run inside the same s_frame and whichever reaches a given
// node first must retire last frame's records so the second finds them already in place. Recording on
// every pass instead would make the in-between present overwrite the real frame's history and halve
// every streak.
#pragma once
#include <cstdint>
#include <unordered_map>
struct Core;

// One frame's projected screen positions for a node's 32 motes.
struct MoteFrame {
  static constexpr int kMotes = 32;
  float sx[kMotes] = {}, sy[kMotes] = {};
  bool valid[kMotes] = {}; // false = this mote had no drawable position that frame
};

class MoteStreaks {
public:
  // Record this node's live positions for the current logic frame and hand back LAST frame's record
  // (nullptr the first time a node is seen, which is the guest's 0x7FFF7FFF sentinel case — no streak,
  // just seed the history).
  const MoteFrame *submit(Core *c, uint32_t node, const MoteFrame &live);

private:
  std::unordered_map<uint32_t, MoteFrame> mCur, mPrev;
  int mFrame = -1;
};
