// game/render/effect_lerp.cpp — EffectLerp implementation (see effect_lerp.h).
#include "effect_lerp.h"
#include "core.h"
#include "game.h"

const EffectPoints& EffectLerp::resolve(Core* c, uint32_t node, const EffectPoints& live) {
  // One rotation per LOGIC frame. Both fps60 presents run inside the same s_frame (the in-between is
  // built first, the real frame second), so whichever of them reaches a given node first retires last
  // frame's records and the second finds them already in place.
  const int frame = c->game->gpu.s_frame;
  if (mFrame != frame) { mPrev.swap(mCur); mCur.clear(); mFrame = frame; }
  mCur[node] = live;

  // Fps60::mT is the parameter of the pass being built right now — 0.5 for the in-between, 1 for the
  // real frame (and 1 when fps60 is off). At 1, or with no previous record for this node, the live
  // state IS the answer.
  const float t = c->game->fps60.mT;
  auto p = mPrev.find(node);
  if (t >= 1.0f || p == mPrev.end()) return live;

  const EffectPoints& prev = p->second;
  mBlend = live;
  const int n = live.n < prev.n ? live.n : prev.n;
  for (int i = 0; i < n; i++) {
    if (!live.valid[i] || !prev.valid[i]) continue;
    mBlend.x[i] = prev.x[i] + (int)((float)(live.x[i] - prev.x[i]) * t);
    mBlend.y[i] = prev.y[i] + (int)((float)(live.y[i] - prev.y[i]) * t);
    mBlend.z[i] = prev.z[i] + (int)((float)(live.z[i] - prev.z[i]) * t);
  }
  return mBlend;
}
