// area21_sky_gradient.cpp — native picture owner for A0L's early-phase sky gradient.
//
// The reached Area 21 drawer branch is not the tilemap composite described by the old inventory note:
// FUN_8010BE30 sees variant 1 and phase < 4, calls FUN_8010BB64, and returns. That helper emits four
// untextured gouraud quads whose Y origin follows the camera pitch. The guest helper remains responsible
// for its packet-pool and OT writes; this read-only display producer rebuilds only the picture.
#include "area21_sky_gradient_policy.h"

#include "core.h"
#include "game.h"
#include "producer_scope.h"
#include "render.h"
#include "render_queue.h"

#include <cmath>
#include <cstdint>
#include <lucent/log.h>

namespace {

constexpr uint32_t kGuestProducer = 0x8010BB64u;
constexpr uint32_t kPitch = 0x1F8000F0u;
constexpr uint32_t kBackgroundState = 0x800BF870u;
constexpr uint32_t kVariant = 0x800BF871u;
constexpr uint32_t kDispatchGate = 0x800BF873u;
constexpr uint32_t kPhase = 0x800BFA55u;

} // namespace

bool Render::area21SkyGradientActive() const {
  return Area21SkyGradientPolicy::active(
      mCore->mem_r8(kDispatchGate), mCore->mem_r8(kBackgroundState), mCore->mem_r8(kVariant), mCore->mem_r8(kPhase));
}

void Render::area21SkyGradientCapture() {
  const int16_t pitch = mCore->mem_r16s(kPitch);
  if (!mArea21SkyPitchValid) {
    mArea21SkyPitchPrev = pitch;
    mArea21SkyPitchValid = true;
  }
  mArea21SkyPitchCur = pitch;
  mArea21SkyCapturedThisFrame = true;
}

void Render::area21SkyGradientRender(float t) {
  if (!mArea21SkyPitchValid) {
    return;
  }

  Core *c = mCore;
  const int pitch =
      mArea21SkyPitchPrev + (int)std::lround((double)(mArea21SkyPitchCur - mArea21SkyPitchPrev) * (double)t);
  const auto bands = Area21SkyGradientPolicy::bands((int16_t)pitch);

  int width = 320;
  int gpu_vk_wide_engine(Core *), gpu_vk_wide_engine_w(Core *);
  if (gpu_vk_wide_engine(c)) {
    width = gpu_vk_wide_engine_w(c);
  }

  RenderQueue &rq = c->game->activeRq();
  RenderQueue::Space2dScope wideFinal(rq, RQ_2D_WIDE_FINAL);
  ProducerScope producerScope(&c->rsub.producerScope, kGuestProducer, "area21SkyGradientRender");
  c->rsub.diag.beginObject(kBackdropDbgNode);
  const int uv[4] = {0, 0, 0, 0};
  for (const auto &band : bands) {
    const int xs[4] = {0, width, 0, width};
    const int ys[4] = {band.top, band.top, band.bottom, band.bottom};
    const unsigned char rs[4] = {band.topColor.r, band.topColor.r, band.bottomColor.r, band.bottomColor.r};
    const unsigned char gs[4] = {band.topColor.g, band.topColor.g, band.bottomColor.g, band.bottomColor.g};
    const unsigned char bs[4] = {band.topColor.b, band.topColor.b, band.bottomColor.b, band.bottomColor.b};
    rq.push2dQuad(RQ_BACKGROUND,
                  /*order_2d_fg=*/0,
                  xs,
                  ys,
                  uv,
                  uv,
                  rs,
                  gs,
                  bs,
                  /*tp_x=*/0,
                  /*tp_y=*/0,
                  /*mode=*/3,
                  /*raw=*/0,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0,
                  1023,
                  511);
    c->rsub.stats.snCmds++;
  }
  c->rsub.diag.endObject();

  lucent::debug("area21sky",
                "f{} t={:.2f} pitch={} origin={} phase={} width={} quads={}",
                c->game->gpu.s_frame,
                (double)t,
                pitch,
                Area21SkyGradientPolicy::originY((int16_t)pitch),
                c->mem_r8(kPhase),
                width,
                bands.size());
}

void Render::area21SkyGradientSwapPrev() {
  if (mArea21SkyCapturedThisFrame) {
    mArea21SkyPitchPrev = mArea21SkyPitchCur;
  } else {
    mArea21SkyPitchValid = false;
  }
  mArea21SkyCapturedThisFrame = false;
}
