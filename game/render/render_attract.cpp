// game/render/render_attract.cpp — native producer for the DEMO/title ATTRACT substate (sm[0x48]==7).
//
// The "attract" mode is the front-end's demo playback: the live 3D FIELD engine hosted UNDERNEATH the
// resident DEMO stage (engine-live latch 0x800BE258==2). The world it draws is the SAME field world the
// walkable game runs — the same render-walk cluster (0x8003bf00/eec0/b588/bb50/bcf4/c048) executes under
// every frame, the same backdrop drawer (0x8003df04 → state-0 tilemap 0x80115598) paints the sky, the
// same terrain node fn (0x8002AB5C) draws the ground, and the SAME scene camera (scratchpad 0x1F8000F8)
// is populated by the field/demo-playback engine each attract frame. Nothing about the picture is
// stage-specific: the field entity lists (HEADS[3] = 0x800FB168/0x800F2624/0x800F2738), the scene table
// (0x800F2418), the backdrop struct (0x800ED018) and the camera all live at FIXED guest addresses,
// independent of the stage pointer. So the honest attract producer is exactly the field world producer.
//
// This is renderField's body verbatim (mirrored, not factored, to keep the producer self-contained):
// re-arm fps60 tier-1 eligibility (renderTitle clears it at entry), open the read-only DisplayPassGuard,
// draw the native WORLD via sceneNative(), then the cinematic letterbox bars (a no-op for attract, which
// arms no letterbox). No new drawing code, no new object walk — sceneNative already reads every needed
// structure. READ-ONLY (writes only the host render queue; DisplayPassGuard aborts on any guest write).
//
// Reached via a scene-selection trace: classifyScene() → SceneKind::Title (DEMO stage 0x801062E4,
// task0 not in reinit); renderTitle() reads s48 = mem_r16(0x801FE048); s48==7 is the attract substate
// (Demo::s2/s3 outcome-1 or the s3 intro timer sm[0x5a]=450 expiring). Before this producer, s48==7 fell
// through to abortUnimplemented (render_walk.cpp). The 2D 'DEMO' watermark (Font::drawText / 0x80079374)
// is a separate 2D layer, intentionally DEFERRED like the other front-end 2D text — the 3D world is the
// load-bearing producer here. docs/port-progress.md:228-229 ("s7 plays OP.STR/FMV") is STALE/falsified
// (demo.cpp later-186/208 RE + tomba2-clips.md empirical screenshots) — do NOT build an FMV producer.
#include "core.h"
#include "game.h"
#include "render.h"

// #6 DEMO/TITLE ATTRACT (stage 0x801062E4, sm[0x48]==7): the live 3D field WORLD under the DEMO stage —
// terrain + entity/scene tables + objects + backdrop, real per-pixel depth, exactly like the walkable
// field. renderTitle sets fps60.mTier1EligibleCur = FALSE at its top, so re-set it TRUE here (a real 3D
// world → tier-1-eligible → 60fps). The 2D 'DEMO' watermark is a deferred 2D layer (see file banner).
void Render::renderAttract() {
  mCore->game->fps60.mTier1EligibleCur = true;   // native world render runs → fps60 tier-1 may re-render it
  DisplayPassGuard displayPass(mCore->rsub.mode);   // read-only invariant: aborts on any guest write
  sceneNative();
  cineBarsRender();     // cinematic letterbox bars (emits nothing when no cutscene bars are active)
}
