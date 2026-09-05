// Shared internals of the native render path — split out so the geometry-SUBMIT subsystem
// (submit.cpp: poly submitters, the render-command dispatcher, transform/matrix orchestration)
// and the render-list WALK subsystem (render_walk.cpp: ov_scene_native + the master/snapshot/aux list
// walks + per-object render/flush + the native backdrop) can live in separate files while sharing the
// few helpers both need (the per-object diagnostic identity scope and the native generic GT3/GT4
// submit the per-object flush calls).
#pragma once
#include "cfg.h" // cfg_dbg
#include "core.h"
#include "mods.h"  // g_mods
#include <stdio.h> // sil_bbox_log diag fprintf

float proj_obj_center_ord(void);
// class ProjParams (game/render/proj_params.h) — per-Core; brings in camview_valid/proj_camview_world_ord etc.
#include "proj_params.h"
// g_fps60_on retired — read g_mods.fps60 (mods.h)

// g_dbg_render_node retired 2026-07-02 — per-Core Render::mDbgRenderNode (set around each per-object
// dispatch in the native render walk; PER-INSTANCE identity for every prim an object emits, incl.
// billboards rasterized later at the OT walk).
#include "game.h"
#include "render.h"       // Render (needed for cur_render_node below)
#include "render_queue.h" // RenderQueue::emitOrQueue + RQ_WORLD

// cur_render_node moved to the framework header runtime/psx/render_node.h (ot_attr.cpp, framework,
// needs it without the rest of this game header). Included here so the game render path is unchanged.
#include "render_node.h"
#include "scene_kind.h"

// render_field_native_active: true iff pc_render's native field pass (Render::sceneNative,
// game_tomba2.cpp Engine::drawOTag) owns THIS frame's picture — GAME stage,
// free-roam (not SOP intro narration), pc_render (not psx_render), not the oracle. Any OTHER
// picture-producing addition that wants to draw real geometry natively (e.g. cmdListDispatch's
// generic-overlay REDIRECT, perobj_dispatch.cpp) must gate on this SAME condition: outside this
// window the guest OT's full walk (psx_render) is the sole picture source, so an extra native
// draw would double-draw. Deliberately narrower than drawOTag's own `scenenative` diagnostic branch
// (that debug channel stays diagnostic-only; it must not also arm new native draws).
// WHY it is off, when it is off. A bare bool made every "is the native pass running here?" question
// answerable only as "no", which is indistinguishable from "I never looked" — and #103 burned a
// session on exactly that ambiguity. The reason codes are what the redirect diagnostic prints.
enum FieldNativeOff {
  FN_ON = 0,    // the native field pass DOES own this frame's picture
  FN_ORACLE,    // oracle leg
  FN_PSXRENDER, // render path is psx (or gte) — the guest OT walk is the sole picture source
  FN_STAGE,     // the GAME stage overlay is not resident (title/intro/menus)
  FN_NARRATION, // SOP intro narration overlay active
  FN_SUBSCENE,  // #51: an AUTHORED OT sub-scene (sm[0x4c]==3) — the full guest walk draws it
  FN_SAVE_MENU, // GAME.BIN Save/Continue/Load/Quit dialog owns a black-backed picture
};
inline const char *field_native_off_name(int r) {
  switch (r) {
  case FN_ON:
    return "on";
  case FN_ORACLE:
    return "oracle";
  case FN_PSXRENDER:
    return "psx_render";
  case FN_STAGE:
    return "stage!=GAME";
  case FN_NARRATION:
    return "narration";
  case FN_SUBSCENE:
    return "sub-scene(sm4C==3)";
  case FN_SAVE_MENU:
    return "save/continue-menu(s4A==2,s4C=3..8)";
  default:
    return "?";
  }
}

static inline int render_field_native_reason(Core *c) {
  if (c->rsub.mode.psxRender()) {
    return FN_PSXRENDER;
  }
  if (c->mem_r32(0x801FE00Cu) != 0x8010637Cu) {
    return FN_STAGE; // GAME stage resident
  }
  uint32_t task_sm = c->mem_r32(0x1F800138u);
  const auto scene = classifyGameStageScene(
      task_sm ? c->mem_r16(task_sm + 0x4Au) : 0, task_sm ? c->mem_r16(task_sm + 0x4Cu) : 0, c->mem_r32(0x80109450u));
  if (scene == GameStageSceneKind::SopNarration) {
    return FN_NARRATION;
  }
  if (scene == GameStageSceneKind::HutInterior) {
    return FN_SUBSCENE;
  }
  if (scene == GameStageSceneKind::SaveContinueMenu) {
    return FN_SAVE_MENU;
  }
  return FN_ON;
}

static inline bool render_field_native_active(Core *c) {
  return render_field_native_reason(c) == FN_ON;
}

// ObjScope — declare "this object is drawing" for the span of a per-node dispatch, so every prim emitted
// beneath it carries the owning node (RqItem::dbg_node) instead of arriving anonymous. Restores the
// previous node rather than calling endObject(), which clears to 0 and would drop an enclosing scope.
// Host-side scope state only (RenderDiag) — never guest memory, so wrapping a byte-exact walk is free.
class ObjScope {
public:
  ObjScope(Core *c, uint32_t node) : mCore(c), mPrev(c->rsub.diag.currentNode()) {
    c->rsub.diag.beginObject(node);
  }
  ~ObjScope() {
    mCore->rsub.diag.beginObject(mPrev);
  }
  ObjScope(const ObjScope &) = delete;
  ObjScope &operator=(const ObjScope &) = delete;

private:
  Core *mCore;
  uint32_t mPrev;
};

// withObjScope — run a per-object draw body under ObjScope so every prim it emits carries the owning
// node as its diagnostic identity (RqItem::dbg_node — objid overlay). Host-side RenderDiag only.
static inline void withObjScope(Core *c, uint32_t node, void (*body)(Core *)) {
  ObjScope scope(c, node);
  body(c);
}

// Fully-native generic GT3/GT4 submit is Render::gt3gt4 (submit.cpp); the per-object flush in
// the walk calls it directly. Scene-table (0x800F2418) world-coord render is Render::fieldEntityRender.

// DIAG (debug channel "silbbox", scratch/handoff.md 2026-07-01 "dark outline" investigation): log the
// screen bbox of any drawn quad overlapping the known repro window (coastal-ridge dark silhouette line,
// pixel-measured x=5..30 y=134-138 — see the handoff). Every quad submitter (native_terrain, the GT3/GT4
// library, the byte-packed variant, and the sky/backdrop tilemap) should call this so the next session can
// see which pass(es) DO or DON'T cover that region — the hypothesis is a sub-pixel coverage gap between
// the hillside object's quad(s) and the sky backdrop letting the black clear color show through.
static inline void sil_bbox_log(const char *tag, const float *px, const float *py, int n) {
  if (!cfg_dbg("silbbox")) {
    return;
  }
  float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
  for (int i = 0; i < n; i++) {
    if (px[i] < minx) {
      minx = px[i];
    }
    if (px[i] > maxx) {
      maxx = px[i];
    }
    if (py[i] < miny) {
      miny = py[i];
    }
    if (py[i] > maxy) {
      maxy = py[i];
    }
  }
  if (maxx < -20 || minx > 160 || maxy < 100 || miny > 200) {
    return; // outside the repro window, skip
  }
  cfg_logi("silbbox", "%s bbox x=[%.1f,%.1f] y=[%.1f,%.1f]", tag, minx, maxx, miny, maxy);
}
// Same, but also identifies WHICH entity node emitted the quad (cur_render_node(c) at call time) — use
// at per-object submit sites so overlapping bboxes at the repro window can be traced back to the object.
static inline void sil_bbox_log_node(const char *tag, const float *px, const float *py, int n, uint32_t node) {
  if (!cfg_dbg("silbbox")) {
    return;
  }
  float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
  for (int i = 0; i < n; i++) {
    if (px[i] < minx) {
      minx = px[i];
    }
    if (px[i] > maxx) {
      maxx = px[i];
    }
    if (py[i] < miny) {
      miny = py[i];
    }
    if (py[i] > maxy) {
      maxy = py[i];
    }
  }
  if (maxx < -20 || minx > 160 || maxy < 100 || miny > 200) {
    return;
  }
  cfg_logi("silbbox", "%s node=%08X bbox x=[%.1f,%.1f] y=[%.1f,%.1f]", tag, node, minx, maxx, miny, maxy);
}
static inline void sil_bbox_log_i(const char *tag, const int *xs, const int *ys, int n) {
  if (!cfg_dbg("silbbox")) {
    return;
  }
  float pxf[8], pyf[8];
  n = n > 8 ? 8 : n;
  for (int i = 0; i < n; i++) {
    pxf[i] = (float)xs[i];
    pyf[i] = (float)ys[i];
  }
  sil_bbox_log(tag, pxf, pyf, n);
}
// Same repro-window gate as sil_bbox_log_node, but also dumps every vertex's screen coord + depth and the
// source record address, so a coverage gap can be told apart from a wrong-color draw (2026-07-01 dark-outline
// direct-inspection pass, scratch/handoff.md).
static inline void sil_bbox_log_verts(const char *tag,
                                      const float *px,
                                      const float *py,
                                      const float *depth,
                                      int n,
                                      uint32_t node,
                                      uint32_t rec_addr,
                                      const uint8_t *r = nullptr,
                                      const uint8_t *g = nullptr,
                                      const uint8_t *b = nullptr) {
  if (!cfg_dbg("silbbox")) {
    return;
  }
  float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
  for (int i = 0; i < n; i++) {
    if (px[i] < minx) {
      minx = px[i];
    }
    if (px[i] > maxx) {
      maxx = px[i];
    }
    if (py[i] < miny) {
      miny = py[i];
    }
    if (py[i] > maxy) {
      maxy = py[i];
    }
  }
  if (maxx < -20 || minx > 160 || maxy < 100 || miny > 200) {
    return;
  }
  CfgLine ln;
  cfg_line_reset(&ln);
  cfg_line_addf(&ln,
                "%s node=%08X rec=%08X bbox x=[%.1f,%.1f] y=[%.1f,%.1f] verts:",
                tag,
                node,
                rec_addr,
                minx,
                maxx,
                miny,
                maxy);
  for (int i = 0; i < n; i++) {
    cfg_line_addf(&ln, " (%.2f,%.2f,z=%.4f", px[i], py[i], depth[i]);
    if (r) {
      cfg_line_addf(&ln, ",rgb=%d,%d,%d", r[i], g[i], b[i]);
    }
    cfg_line_addf(&ln, ")");
  }
  cfg_line_flush(&ln, "silbbox");
}
