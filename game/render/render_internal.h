// Shared internals of the native render path — split out so the geometry-SUBMIT subsystem
// (submit.cpp: poly submitters, the render-command dispatcher, transform/matrix orchestration)
// and the render-list WALK subsystem (render_walk.cpp: ov_scene_native + the master/snapshot/aux list
// walks + per-object render/flush + the native backdrop) can live in separate files while sharing the
// few helpers both need (the per-object diagnostic identity scope and the native generic GT3/GT4
// submit the per-object flush calls).
#pragma once
#include "core.h"
#include "mods.h"   // g_mods
#include "cfg.h"    // cfg_dbg
#include <stdio.h>  // sil_bbox_log diag fprintf

float proj_obj_center_ord(void);
// class ProjParams (game/render/proj_params.h) — per-Core; brings in camview_valid/proj_camview_world_ord etc.
#include "proj_params.h"
// g_fps60_on retired — read g_mods.fps60 (mods.h)

// g_dbg_render_node retired 2026-07-02 — per-Core Render::mDbgRenderNode (set around each per-object
// dispatch in the native render walk; PER-INSTANCE identity for every prim an object emits, incl.
// billboards rasterized later at the OT walk).
#include "render.h"    // Render (needed for cur_render_node below)
#include "game.h"      // c->game->oracle
#include "render_queue.h"  // RenderQueue::emitOrQueue + RQ_WORLD

// cur_render_node moved to the framework header runtime/recomp/render_node.h (ot_attr.cpp, framework,
// needs it without the rest of this game header). Included here so the game render path is unchanged.
#include "render_node.h"

// render_field_native_active: true iff pc_render's native field pass (Render::sceneNative,
// game_tomba2.cpp Engine::drawOTag) owns THIS frame's picture — GAME stage,
// free-roam (not SOP intro narration), pc_render (not psx_render), not the oracle. Any OTHER
// picture-producing addition that wants to draw real geometry natively (e.g. cmdListDispatch's
// generic-overlay REDIRECT, perobj_dispatch.cpp) must gate on this SAME condition: outside this
// window the guest OT's full walk (psx_render) is the sole picture source, so an extra native
// draw would double-draw. Deliberately narrower than drawOTag's own `scenenative` diagnostic branch
// (that debug channel stays diagnostic-only; it must not also arm new native draws).
static inline bool render_field_native_active(Core* c) {
  if (c->game->oracle || c->rsub.mode.psxRender()) return false;
  if (c->mem_r32(0x801FE00Cu) != 0x8010637Cu) return false;         // GAME stage resident
  if (c->mem_r32(0x80109450u) == 0x3C021F80u) return false;         // SOP intro narration overlay active
  // #51: an AUTHORED OT sub-scene (hut/door interior, sm[0x4c]==3 — the game's own fieldRunX/frameX
  // selector; see game_tomba2.cpp) is drawn ENTIRELY by the full guest-OT walk (#49), NOT the native
  // field pass — so the native pass does NOT own the picture here. If this returned true, the
  // cmdListDispatch REDIRECT would draw natively while the walk also draws -> double-draw (and the #48
  // era showed the inverse failure: owned objects vanishing, Tomba invisible).
  uint32_t task_sm = c->mem_r32(0x1F800138u);
  if (task_sm && c->mem_r16(task_sm + 0x4Cu) == 3) return false;
  return true;
}

// ObjScope — declare "this object is drawing" for the span of a per-node dispatch, so every prim emitted
// beneath it carries the owning node (RqItem::dbg_node) instead of arriving anonymous. Restores the
// previous node rather than calling endObject(), which clears to 0 and would drop an enclosing scope.
// Host-side scope state only (RenderDiag) — never guest memory, so wrapping a byte-exact walk is free.
class ObjScope {
public:
  ObjScope(Core* c, uint32_t node) : mCore(c), mPrev(c->rsub.diag.currentNode()) { c->rsub.diag.beginObject(node); }
  ~ObjScope() { mCore->rsub.diag.beginObject(mPrev); }
  ObjScope(const ObjScope&) = delete;
  ObjScope& operator=(const ObjScope&) = delete;
private:
  Core* mCore; uint32_t mPrev;
};

// withObjScope — run a per-object draw body under ObjScope so every prim it emits carries the owning
// node as its diagnostic identity (RqItem::dbg_node — objid overlay). Host-side RenderDiag only.
static inline void withObjScope(Core* c, uint32_t node, void (*body)(Core*)) {
  ObjScope scope(c, node);
  body(c);
}

// ---- WqRec factorization helpers (#67 display-pass quads — render.h WqRec banner) ------------------
// wq_read_matrix: read a libgte MATRIX (m 9×s16 CR-packed in 5 words @+0, t 3×s32 @+0x14) from guest
// memory into unit-scale float form — the same packing SetRotMatrix/SetTransMatrix load into CR0-7.
static inline void wq_read_matrix(Core* c, uint32_t m, float crF[3][3], float tr[3]) {
  constexpr float FX = 1.0f / 4096.0f;
  uint32_t w0 = c->mem_r32(m + 0), w1 = c->mem_r32(m + 4), w2 = c->mem_r32(m + 8),
           w3 = c->mem_r32(m + 12), w4 = c->mem_r32(m + 16);
  crF[0][0] = (int16_t)w0 * FX;         crF[0][1] = (int16_t)(w0 >> 16) * FX; crF[0][2] = (int16_t)w1 * FX;
  crF[1][0] = (int16_t)(w1 >> 16) * FX; crF[1][1] = (int16_t)w2 * FX;         crF[1][2] = (int16_t)(w2 >> 16) * FX;
  crF[2][0] = (int16_t)w3 * FX;         crF[2][1] = (int16_t)(w3 >> 16) * FX; crF[2][2] = (int16_t)w4 * FX;
  for (int i = 0; i < 3; i++) tr[i] = (float)(int32_t)c->mem_r32(m + 0x14u + (uint32_t)i * 4u);
}
// wq_factor_world: factor a composed transform against the SCRATCHPAD scene camera (pure at guest
// emit time — per-object composes touch only the GTE regs): CR = cam∘obj, tr = cam·pos + cam.t ⇒
// objR = camᵀ·CR, objT = camᵀ·(tr − camT). Exact at the endpoints for ANY CR content (orthonormal
// camera ⇒ inverse = transpose); the display pass re-composes with the (fps60-lerped) camera.
static inline void wq_factor_world(Core* c, const float crF[3][3], const float tr[3],
                                   float objR[3][3], float objT[3]) {
  float camR[3][3], camT[3];
  wq_read_matrix(c, 0x1F8000F8u, camR, camT);   // the scene camera MATRIX (same bytes sceneCam reads)
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      float s = 0; for (int k = 0; k < 3; k++) s += camR[k][i] * crF[k][j];   // camᵀ·CR
      objR[i][j] = s;
    }
    float s = 0; for (int k = 0; k < 3; k++) s += camR[k][i] * (tr[k] - camT[k]);
    objT[i] = s;
  }
}

// Fully-native generic GT3/GT4 submit is Render::gt3gt4 (submit.cpp); the per-object flush in
// the walk calls it directly. Scene-table (0x800F2418) world-coord render is Render::fieldEntityRender.

// DIAG (debug channel "silbbox", scratch/handoff.md 2026-07-01 "dark outline" investigation): log the
// screen bbox of any drawn quad overlapping the known repro window (coastal-ridge dark silhouette line,
// pixel-measured x=5..30 y=134-138 — see the handoff). Every quad submitter (native_terrain, the GT3/GT4
// library, the byte-packed variant, and the sky/backdrop tilemap) should call this so the next session can
// see which pass(es) DO or DON'T cover that region — the hypothesis is a sub-pixel coverage gap between
// the hillside object's quad(s) and the sky backdrop letting the black clear color show through.
static inline void sil_bbox_log(const char* tag, const float* px, const float* py, int n) {
  if (!cfg_dbg("silbbox")) return;
  float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
  for (int i = 0; i < n; i++) {
    if (px[i] < minx) minx = px[i]; if (px[i] > maxx) maxx = px[i];
    if (py[i] < miny) miny = py[i]; if (py[i] > maxy) maxy = py[i];
  }
  if (maxx < -20 || minx > 160 || maxy < 100 || miny > 200) return;   // outside the repro window, skip
  cfg_logi("silbbox", "%s bbox x=[%.1f,%.1f] y=[%.1f,%.1f]", tag, minx, maxx, miny, maxy);
}
// Same, but also identifies WHICH entity node emitted the quad (cur_render_node(c) at call time) — use
// at per-object submit sites so overlapping bboxes at the repro window can be traced back to the object.
static inline void sil_bbox_log_node(const char* tag, const float* px, const float* py, int n, uint32_t node) {
  if (!cfg_dbg("silbbox")) return;
  float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
  for (int i = 0; i < n; i++) {
    if (px[i] < minx) minx = px[i]; if (px[i] > maxx) maxx = px[i];
    if (py[i] < miny) miny = py[i]; if (py[i] > maxy) maxy = py[i];
  }
  if (maxx < -20 || minx > 160 || maxy < 100 || miny > 200) return;
  cfg_logi("silbbox", "%s node=%08X bbox x=[%.1f,%.1f] y=[%.1f,%.1f]", tag, node, minx, maxx, miny, maxy);
}
static inline void sil_bbox_log_i(const char* tag, const int* xs, const int* ys, int n) {
  if (!cfg_dbg("silbbox")) return;
  float pxf[8], pyf[8]; n = n > 8 ? 8 : n;
  for (int i = 0; i < n; i++) { pxf[i] = (float)xs[i]; pyf[i] = (float)ys[i]; }
  sil_bbox_log(tag, pxf, pyf, n);
}
// Same repro-window gate as sil_bbox_log_node, but also dumps every vertex's screen coord + depth and the
// source record address, so a coverage gap can be told apart from a wrong-color draw (2026-07-01 dark-outline
// direct-inspection pass, scratch/handoff.md).
static inline void sil_bbox_log_verts(const char* tag, const float* px, const float* py, const float* depth,
                                       int n, uint32_t node, uint32_t rec_addr,
                                       const uint8_t* r = nullptr, const uint8_t* g = nullptr, const uint8_t* b = nullptr) {
  if (!cfg_dbg("silbbox")) return;
  float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
  for (int i = 0; i < n; i++) {
    if (px[i] < minx) minx = px[i]; if (px[i] > maxx) maxx = px[i];
    if (py[i] < miny) miny = py[i]; if (py[i] > maxy) maxy = py[i];
  }
  if (maxx < -20 || minx > 160 || maxy < 100 || miny > 200) return;
  CfgLine ln; cfg_line_reset(&ln);
  cfg_line_addf(&ln, "%s node=%08X rec=%08X bbox x=[%.1f,%.1f] y=[%.1f,%.1f] verts:",
                tag, node, rec_addr, minx, maxx, miny, maxy);
  for (int i = 0; i < n; i++) {
    cfg_line_addf(&ln, " (%.2f,%.2f,z=%.4f", px[i], py[i], depth[i]);
    if (r) cfg_line_addf(&ln, ",rgb=%d,%d,%d", r[i], g[i], b[i]);
    cfg_line_addf(&ln, ")");
  }
  cfg_line_flush(&ln, "silbbox");
}
