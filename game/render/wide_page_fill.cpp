// class WidePageFill — implementation. See wide_page_fill.h for the measurement that identified the
// uncovered margin and for why the framework's flat-untextured stretch rule does not reach it.
#include "wide_page_fill.h"

#include "core.h"
#include "gpu_vk.h"
#include "producer_scope.h"
#include "render_queue.h"

namespace tomba::render {

namespace {
// The authored screen height every full-screen page in this title fills, and the one the guest's own
// full-screen quads use (pause_menu.cpp's 320x240 tile, render_options.cpp's 0..240 gradient).
constexpr int kPageHeight = 240;
} // namespace

int WidePageFill::canvasWidth(Core &core) {
  const int wide = gpu_vk_wide_engine(&core) ? gpu_vk_wide_engine_w(&core) : 0;
  const int native = gpu_vk_native_w(&core);
  return wide > native ? wide : native;
}

bool WidePageFill::widened(Core &core) {
  return canvasWidth(core) > gpu_vk_native_w(&core);
}

void WidePageFill::pushBehindPage(Core &core, RenderQueue &queue, int layer, int order2dFg) {
  if (!widened(core)) {
    return;
  }
  // A PC-only prim: the page it backs has no guest counterpart for it, so a guest-keyed scope here
  // would add one native prim against the guest's one and make a faithful producer read 2-vs-1 in the
  // one column the census exists to compare (producer_scope.h records that exact case). One id for
  // one enhancement — the two private copies this replaces each named their own, which is how two
  // rows for one behaviour start.
  ProducerScope scope(&core.rsub.producerScope, pc_producer("pc/wide-page-fill"));
  // WIDE-FINAL: these x values are the canvas's own, not a 4:3 author's, so the queue's centring must
  // leave them alone. Declaring the space is the mechanism; inferring it from the material is what
  // left the two earlier copies painting inside the page instead of beside it.
  RenderQueue::Space2dScope wideFinal(queue, RQ_2D_WIDE_FINAL);
  const int right = canvasWidth(core);
  int xs[4] = {0, right, 0, right};
  int ys[4] = {0, 0, kPageHeight, kPageHeight};
  int z[4] = {0, 0, 0, 0};
  unsigned char black[4] = {0, 0, 0, 0};
  queue.push2dQuad(layer,
                   order2dFg,
                   xs,
                   ys,
                   z,
                   z,
                   black,
                   black,
                   black,
                   /*tp_x=*/0,
                   /*tp_y=*/0,
                   /*mode=*/3,
                   /*raw=*/0,
                   /*clut_x=*/0,
                   /*clut_y=*/0,
                   /*tw_mx=*/0,
                   /*tw_my=*/0,
                   /*tw_ox=*/0,
                   /*tw_oy=*/0,
                   /*da_x0=*/0,
                   /*da_y0=*/0,
                   /*da_x1=*/1023,
                   /*da_y1=*/511);
}

} // namespace tomba::render
