// game/render/render_hut_interior.cpp — pc_render producer for the HUT/DOOR authored sub-scene
// (GAME field stage 0x8010637C, task-sm[0x4c]==3; classifyScene() == SceneKind::HutInterior).
//
// REDUCED-WORLD producer: OBJECTS ONLY — no exterior terrain / scene-table / backdrop. The interior is a
// seamless door swap INSIDE the village area (scene-active 0x800BE258 stays 2), so the substrate keeps the
// field WORLD data resident and still pointing at the VILLAGE exterior (terrain node, PARALLAX_BG_SM
// tilemap 0x800ED018, SCENE_ENT_TABLE 0x800F2418). Drawing those passes through the now-interior camera
// paints the village behind the room — the original "exterior leak" bug (docs/findings/render.md
// "Hut interior FIXED 2026-07-15"). The authored interior lives in the SAME entity lists the field object
// pass already walks:
//   · interior ROOM node 0x800FD850 (behavior 0x8012C910) — a HEADS[1] object
//   · NPCs / hanging props — HEADS[0..2] objects
//   · Tomba — the G block (ActorTomba::G_ADDR), outside the 3 lists
// fieldObjectsRender() walks exactly that set (perObjFlush -> projComposeObject -> gt3gt4, real per-pixel
// depth) under the LIVE interior camera: the guest writes the interior view matrix to scratchpad CAM2
// (0x1F8000F8) + GTE ctrl OFX/OFY/H during the state-3 tick, read through the Fps60::sceneCam choke — so
// the camera comes for FREE, no producer-side camera code. This mirrors the substrate's reduced frameX
// pass (guest 0x8003FA44) but REBUILDS the picture from object data — it does NOT transcribe the OT /
// the 0x80146478 overlay-geometry packets.
//
// tier1-eligible: the reduced object set is identical every present, so fps60 re-runs it under lerped
// per-object transforms for a stable 60fps interior. The interp re-run (tomba_fps60_world_pass) gates its
// exterior passes off when classifyScene()==HutInterior, so the in-between frames draw the SAME reduced
// object set — no village flicker (docs/findings/render.md "fps60 HUT-INTERIOR FLICKER").
//
// READ-ONLY overlay: reads guest RAM (entity lists / object nodes / scratchpad camera) + engine classes,
// writes ONLY the host render queue. DisplayPassGuard aborts on any guest write. No GTE compose, no
// gte_op, no OT/GP0 packet reading — the picture is rebuilt from object data with real depth.
#include "core.h"
#include "game.h"        // c->game->fps60 — tier1 eligibility flag
#include "game_ctx.h"    // rend(c)
#include "render.h"

// #4 HUT/DOOR INTERIOR (task-sm[0x4c]==3): OBJECTS-ONLY reduced world. See file banner.
void Render::renderHutInterior() {
  mCore->game->fps60.mTier1EligibleCur = true;      // reduced object set is stable per present -> tier1 60fps
  DisplayPassGuard displayPass(mCore->rsub.mode);   // read-only invariant: aborts on any guest write
  // Room 0x800FD850 + NPCs/props (HEADS[0..2]) + Tomba (G block) via perObjFlush -> gt3gt4, real per-pixel
  // depth, live interior camera. Deliberately NO terrainRenderAll / fieldEntityRender(0x800F2418) /
  // backdropRender — those are the VILLAGE-exterior data; drawing them through the interior camera IS the
  // exterior-leak bug this reduced pass exists to avoid.
  fieldObjectsRender();
  cineBarsRender();     // door-transition letterbox (emits nothing when no cutscene bars are active)
}