// class PageBackdrop — implementation. See page_backdrop.h for the measurement that identified the
// uncovered margin, and page_gradient.h for the authored gradient and the band rule this applies.
#include "page_backdrop.h"

#include "core.h"
#include "gpu_vk.h"
#include "page_gradient.h"
#include "producer_scope.h"
#include "render_queue.h"

namespace tomba::render {

namespace {

// The one quad push both the authored page and its margin bands go through: untextured (mode 3),
// full draw area, per-vertex colour in TL, TR, BL, BR order.
void pushQuad(RenderQueue &queue,
              int layer,
              int order2dFg,
              const int *xs,
              const int *ys,
              const unsigned char *rs,
              const unsigned char *gs,
              const unsigned char *bs) {
  int z[4] = {0, 0, 0, 0};
  queue.push2dQuad(layer,
                   order2dFg,
                   xs,
                   ys,
                   z,
                   z,
                   rs,
                   gs,
                   bs,
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

// Draw one margin band: the edge's top colour across both top vertices and its bottom colour across
// both bottom vertices, so the band is constant horizontally — the clamp continuation itself.
void pushBand(RenderQueue &queue, int layer, int order2dFg, const PageMarginBand &band) {
  if (band.x1 <= band.x0) {
    return;
  }
  int xs[4] = {band.x0, band.x1, band.x0, band.x1};
  int ys[4] = {0, 0, kPageHeight, kPageHeight};
  unsigned char rs[4] = {band.color.top.r, band.color.top.r, band.color.bottom.r, band.color.bottom.r};
  unsigned char gs[4] = {band.color.top.g, band.color.top.g, band.color.bottom.g, band.color.bottom.g};
  unsigned char bs[4] = {band.color.top.b, band.color.top.b, band.color.bottom.b, band.color.bottom.b};
  pushQuad(queue, layer, order2dFg, xs, ys, rs, gs, bs);
}

} // namespace

int PageBackdrop::canvasWidth(Core &core) {
  const int wide = gpu_vk_wide_engine(&core) ? gpu_vk_wide_engine_w(&core) : 0;
  const int native = gpu_vk_native_w(&core);
  return wide > native ? wide : native;
}

bool PageBackdrop::widened(Core &core) {
  return canvasWidth(core) > gpu_vk_native_w(&core);
}

void PageBackdrop::pushAuthored(Core &core, RenderQueue &queue, int layer, int order2dFg, const PageGradient &page) {
  (void)core;
  // Authored 4:3 coordinates: the queue centres them in the widened frame, which is what puts the
  // page exactly between the two margin bands pushMargins computes.
  int xs[4] = {0, kPageWidth, 0, kPageWidth};
  int ys[4] = {0, 0, kPageHeight, kPageHeight};
  const Rgb corners[4] = {page.topLeft(), page.topRight(), page.bottomLeft(), page.bottomRight()};
  unsigned char rs[4];
  unsigned char gs[4];
  unsigned char bs[4];
  for (int i = 0; i < 4; i++) {
    rs[i] = corners[i].r;
    gs[i] = corners[i].g;
    bs[i] = corners[i].b;
  }
  pushQuad(queue, layer, order2dFg, xs, ys, rs, gs, bs);
}

void PageBackdrop::pushMargins(Core &core, RenderQueue &queue, int layer, int order2dFg, const PageGradient &page) {
  PageMarginBand bands[kMaxPageMarginBands];
  const int count = pageMarginBands(canvasWidth(core), gpu_vk_native_w(&core), page, bands);
  if (count == 0) {
    return; // 4:3 — the page is already the whole picture
  }
  // A PC-only prim: the page it backs has no guest counterpart for it, so a guest-keyed scope here
  // would add native prims against the guest's one and make a faithful producer read wrong in the
  // one column the census exists to compare (producer_scope.h records that exact case). One id for
  // one enhancement — the two private copies this replaces each named their own, which is how two
  // rows for one behaviour start.
  ProducerScope scope(&core.rsub.producerScope, pc_producer("pc/wide-page-fill"));
  // WIDE-FINAL: these x values are the canvas's own, not a 4:3 author's, so the queue's centring must
  // leave them alone. Declaring the space is the mechanism; inferring it from the material is what
  // left the two earlier copies painting inside the page instead of beside it. It also keeps the
  // left band — flat and untextured — off the framework's RQ_BACKGROUND stretch path, which would
  // otherwise spread that one margin across the whole canvas.
  RenderQueue::Space2dScope wideFinal(queue, RQ_2D_WIDE_FINAL);
  for (int i = 0; i < count; i++) {
    pushBand(queue, layer, order2dFg, bands[i]);
  }
}

} // namespace tomba::render
