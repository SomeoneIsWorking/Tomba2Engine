// game/render/effect_lerp.h — EffectLerp: the ACTOR-TRANSFORM interpolation tier for effect nodes.
//
// CLAUDE.md "LERP IS NATIVE TOO": an interpolated frame is a host-side presentation concern, so a native
// producer interpolates the per-object state it OWNS and then draws, using the same render path for the
// real and the in-between frame. Fps60 already does this for the two inputs the generic object walk reads
// (the scene camera, and each render command's world transform, keyed by cmd). Effect nodes have neither:
// their geometry comes from WORLD POINTS the effect itself keeps (a dust trail's position ring, a sprite
// effect's anchor), read straight out of the node. This is the same capture-then-lerp choke for those.
//
// Per logic frame the producer hands its live world points in; they are recorded under the node address
// and returned unchanged on the real pass, and on the fps60 in-between present the previous frame's record
// for the same node is blended in at Fps60::mT. Host memory only — nothing here reads or writes anything
// the guest can see, and a node with no previous record simply draws at its live state.
//
// Why lerping a ring INDEX-WISE is right: the trail's slot i holds the position from i frames ago, so
// between frame N and N+1 every slot advances by exactly one sample. Blending slot i against slot i of
// the previous frame therefore yields the polyline at time N + t, which is precisely the in-between.
#pragma once
#include <stdint.h>
#include <unordered_map>
struct Core;

// The world points one effect node contributes this frame (dust: up to 4 ring points + the puff origin;
// a sprite effect: its single anchor). Counts are per-producer and stable for a given node.
struct EffectPoints {
  static constexpr int kMax = 8;
  int   n = 0;
  int   x[kMax] = {}, y[kMax] = {}, z[kMax] = {};
  bool  valid[kMax] = {};
};

class EffectLerp {
public:
  // Resolve this node's points for the pass currently running. Records `live` as this frame's endpoint
  // and returns it on a real frame; returns the blend with last frame's record on the in-between present.
  const EffectPoints& resolve(Core* c, uint32_t node, const EffectPoints& live);

private:
  std::unordered_map<uint32_t, EffectPoints> mCur, mPrev;
  EffectPoints mBlend;
  int mFrame = -1;
};
