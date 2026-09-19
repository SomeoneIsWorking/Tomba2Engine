// game/render/fps60_worldpass.cpp — TRANSITIONAL fps60 world-pass seam (see game_iface.h death condition).
//
// The interpolated-60fps lerp tier (Fps60) is a GENERIC renderer feature and lives framework-side
// (runtime/psx/fps60.cpp). Its interp present RE-RUNS the game's world passes one frame behind, under
// the framework Fps60's lerped inputs, into its isolated sink. That re-run is the ONE place the framework
// still reaches into game render — carried here, behind the fps60WorldPass hook. The temporal-rotation
// hook below advances game-owned producer inputs after both presentation slots.
//
// TARGET (USER 2026-07-17): the game submits its drawables to the framework once and the framework lerps
// them directly — no callback into game render. Delete this file + both hooks when that submit model lands.
//
// The projParams/rqRedirect/camera-override/DisplayPassGuard scaffold stays framework-side (Fps60::
// tier1Render); this body is only the gate reads + the world-pass draws, plus the backdrop wrap-lerp that
// writes the framework Fps60's (public) bg-override state that Render::backdropRender reads back.
#include "core.h"
#include "fps60.h" // Fps60 — the override struct fields
#include "game.h"
#include "game_ctx.h"        // rend(c) / eng(c) — the game's Render and engine object graphs
#include "parallax_bg.h"     // ParallaxBg — owns the SM address and the scroll wrap moduli
#include "parallax_scroll.h" // tomba::parallax::shortestPathLerp — the scroll-domain owner
#include "render.h" // Render::worldVoidBeat/fieldAreaInit/terrainRenderAll/fieldEntityRender/backdropRender/...

void tomba_fps60_world_pass(Core *c, float t) {
  Fps60 &f = fps60(*c->game);
  // #67 GATE PARITY: mirror the REAL frame's world-pass gates (Render::worldVoidBeat / fieldAreaInit —
  // the same reads sceneNative made this interval). Re-running past a gate the real frame honored paints
  // that pass on interp presents only (30Hz flicker of the whole layer).
  const bool voidBeat = rend(c)->worldVoidBeat();
  const bool areaInit = rend(c)->fieldAreaInit();
  // HUT-INTERIOR PARITY: renderHutInterior is a reduced OBJECTS-ONLY sub-scene that skips the exterior
  // terrain/scene-table/backdrop (they still point at the VILLAGE). Mirror that gate here, or the interp
  // presents redraw the village exterior on the in-between frames (documented interior flicker). The
  // object walk below stays UNGATED so the interp frames draw the identical reduced object set.
  const bool hutInterior = rend(c)->classifyScene() == Render::SceneKind::HutInterior;
  if (!voidBeat && !areaInit && !hutInterior) {
    rend(c)->terrainRenderAll();
  }
  // SCENE TABLE (grass/terrain props): camera-only, same gate as terrain (mSceneTableTrusted, per the
  // present-time invariant — no tick has run since the real frame computed it).
  if (!voidBeat && !areaInit && !hutInterior && rend(c)->mSceneTableTrusted) {
    rend(c)->fieldEntityRender(0x800F2418u);
  }
  // BACKDROP (game-logic scroll, LAYER-TRANSFORM lerp — not camera-projected): mirrors sceneNative's own
  // gate (mBackdropTrusted && the resident drawer is the shared tilemap routine — seaside + areas 10/11,
  // kanban #42). The wrap moduli are static per-area config, read from their owner (ParallaxBg), and the
  // lerp is the scroll domain's own (parallax_scroll.h) — NOT the reduction ParallaxBg::step applies to a
  // real frame, which is a different function on purpose. See that header.
  int bgVAdd;
  if (!voidBeat && !hutInterior && rend(c)->mBackdropTrusted && rend(c)->backdropTilemapDrawer(bgVAdd)) {
    const ParallaxBg &bg = eng(c).parallaxBg;
    f.mBgOverride.scrollX = tomba::parallax::shortestPathLerp(f.mBgPrev.scrollX, f.mBgCur.scrollX, bg.scrollModX(), t);
    f.mBgOverride.scrollY = tomba::parallax::shortestPathLerp(f.mBgPrev.scrollY, f.mBgCur.scrollY, bg.scrollModY(), t);
    f.mBgOverrideOn = true;
    rend(c)->backdropRender(ParallaxBg::SM_ADDR);
    f.mBgOverrideOn = false;
  }
  // Area 21's reached variant-1/early-phase branch is the four-quad gradient helper and returns before
  // the tilemap loop. Rebuild it from the raw pitch captured by the real scene pass; the producer owns
  // the interpolation slot because PARALLAX_BG_SM's wrapped scroll is not the helper's input.
  if (!voidBeat && !hutInterior && rend(c)->mBackdropTrusted && rend(c)->area21SkyGradientActive()) {
    rend(c)->area21SkyGradientRender(t);
  }
  // Field OBJECT walk under lerped per-object transforms (mObjOverrideOn + the captured projObj) AND the
  // still-armed lerped camera into the sink. Objects run on the void beat (vortex node) like the real
  // frame; only areaInit suppresses them (mirrors sceneNative's field_area_init block).
  if (!areaInit) {
    f.mObjOverrideOn = true;
    rend(c)->fieldObjectsRender();
    f.mObjOverrideOn = false;
  }
}

void tomba_fps60_temporal_rotate(Core *c) {
  rend(c)->area21SkyGradientSwapPrev();
}
