// Tomba! 2 title-screen wide composition.
//
// The retail title artwork is one authored 320x240 picture split across two 8bpp texture pages.
// Native 2D layout correctly centers textured content in a wider canvas, which preserves that art but
// leaves black pillars. This owner fills only the added canvas: it mirrors the picture's own outer
// 64-pixel strips, darkened into peripheral panels. The central art remains byte-for-byte the same
// size and framing; no final-present texture is stretched, cropped, or post-processed.

#include "core.h"
#include "game.h"
#include "gpu_vk.h"
#include "producer_scope.h"
#include "render.h"
#include "render_queue.h"

#include <algorithm>

namespace {

constexpr int kTitleWidth = 320;
constexpr int kTitleHeight = 240;
constexpr int kTitleTop = -8;
constexpr int kEdgeTextureWidth = 64;
constexpr int kEdgeTextureMaxU = kEdgeTextureWidth - 1;
constexpr unsigned char kPeripheralLevel = 0x50;

int mirroredEdgeU(int distance, bool rightEdge) {
  const int phase = distance % (kEdgeTextureMaxU * 2);
  const int fromLeft = phase <= kEdgeTextureMaxU ? phase : kEdgeTextureMaxU * 2 - phase;
  return rightEdge ? kEdgeTextureMaxU - fromLeft : fromLeft;
}

void emitEdgeStrip(Core &core, bool rightEdge, int margin) {
  RenderQueue &queue = core.game->activeRq();
  const int texturePageX = rightEdge ? 768 : 640;
  int distance = 0;
  while (distance < margin) {
    const int nextBoundary = ((distance / kEdgeTextureMaxU) + 1) * kEdgeTextureMaxU;
    const int nextDistance = std::min(margin, nextBoundary);
    const int nearX = rightEdge ? kTitleWidth + distance : -distance;
    const int farX = rightEdge ? kTitleWidth + nextDistance : -nextDistance;
    const int nearU = mirroredEdgeU(distance, rightEdge);
    const int farU = mirroredEdgeU(nextDistance, rightEdge);
    int xs[4] = {farX, nearX, farX, nearX};
    int ys[4] = {kTitleTop, kTitleTop, kTitleTop + kTitleHeight, kTitleTop + kTitleHeight};
    int us[4] = {farU, nearU, farU, nearU};
    int vs[4] = {0, 0, kTitleHeight, kTitleHeight};
    unsigned char level[4] = {kPeripheralLevel, kPeripheralLevel, kPeripheralLevel, kPeripheralLevel};
    queue.push2dQuad(RQ_BACKGROUND,
                     /*order_2d_fg=*/1,
                     xs,
                     ys,
                     us,
                     vs,
                     level,
                     level,
                     level,
                     texturePageX,
                     256,
                     /*mode=*/1,
                     /*raw=*/0,
                     640,
                     511,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     1023,
                     511);
    distance = nextDistance;
  }
}

} // namespace

void Render::titleWideMargins() {
  Core *core = mCore;
  if (!gpu_vk_wide_engine(core)) {
    return;
  }
  const int nativeWidth = core->game->gpu.s_disp_w > 0 ? core->game->gpu.s_disp_w : kTitleWidth;
  const int margin = (gpu_vk_wide_engine_w(core) - nativeWidth) / 2;
  if (margin <= 0) {
    return;
  }

  // These panels are a PC-only enhancement, not work performed by guest FUN_80106690. Keep their
  // producer identity independent even though menuChrome owns the surrounding authored picture.
  ProducerScope wideScope(&core->rsub.producerScope, pc_producer("pc/title-wide-margins"));
  emitEdgeStrip(*core, false, margin);
  emitEdgeStrip(*core, true, margin);
}
