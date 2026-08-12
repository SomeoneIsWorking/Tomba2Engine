// game/render/render_attract.cpp — native producer for the DEMO/title ATTRACT substate (sm[0x48]==7).
//
// The "attract" mode is the front-end's demo playback: the live 3D FIELD engine hosted UNDERNEATH the
// resident DEMO stage. The world it draws is the SAME field world the walkable game runs — the same
// render-walk cluster (0x8003bf00/eec0/b588/bb50/bcf4/c048) executes under every frame, the same backdrop
// drawer (0x8003df04 → state-0 tilemap 0x80115598) paints the sky, the same terrain node fn (0x8002AB5C)
// draws the ground, and the SAME scene camera (scratchpad 0x1F8000F8) is populated by the field/demo-
// playback engine each attract frame. Nothing about the picture is stage-specific: the field entity lists
// (HEADS[3] = 0x800FB168/0x800F2624/0x800F2738), the scene table (0x800F2418), the backdrop struct
// (0x800ED018) and the camera all live at FIXED guest addresses, independent of the stage pointer. So the
// honest attract producer is exactly the field world producer — BUT ONLY WHILE THE ATTRACT ITEM IS LIVE,
// which is a state, not a property of the substate. See attractItemLive() below: s48==7 spans the item's
// LAUNCH frames too, and on those the world does not exist yet.
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
#include <lucent/log.h>   // `attract` diagnostic channel (the gate decision + its denominators)

// attractItemLive — IS THERE AN ATTRACT WORLD THIS FRAME? (kanban #86, the fix for the pre-existing
// "UNMAPPED RAM read8 @ 0x07035D41 in fieldObjectsRender <- renderAttract" abort.)
//
// s48==7 is the attract SUBSTATE, not the attract WORLD. The substate runs the 3-phase machine
// 0x80106C24 (RE'd in game/scene/demo.cpp `Demo::s7Phase`, later-186): phase0 LAUNCHES an item — selects
// it, streams its area off the CD through the cooperative slot-1 area load, then places objects and
// reinitialises the field subsystems; phase1 plays that item for 900 frames; phase2 tears the item down
// and restarts the front-end at substate 0. The entity lists only hold the item's nodes from the END of
// phase0 to the teardown — and phase0 YIELDS (the area load is cooperative), so s48 reads 7 for one to
// two frames BEFORE any of it exists, on every cycle.
//
// On those launch frames the three HEADS still hold the PREVIOUS item's nodes, whose memory phase2's
// teardown released and the incoming area stream is now overwriting — so the walk follows a dead node's
// link field into whatever the loader wrote there. MEASURED (attract-cycle 2, `attract` channel): head[0]
// = 0x800EF478 survives the teardown; its link word (node+0x24) reads 0x800EF53C while the item is live,
// 0x00000000 on the first launch frame (teardown zeroed it), and 0x07035D40 on the second — raw file data
// landed on the dead node — at which point fieldObjectsRender's first field read, mem_r8(n+1), leaves the
// 2 MB of RAM and the memory model fails fast. Nothing is wrong with the pointer CHECK; the walk simply
// must not run in this state, which is why this is a state gate and not a range test on n.
//
// THE LATCH: *(u8)0x1F80019A is the phase machine's OWN "this item's world is built" flag. VERIFIED at
// instruction level against the real overlay, not inferred — `tools/disasm_overlay.py
// scratch/bin/overlays/DEMO.BIN`:
//   0x80106D1C  sb $s1,0x19a($v0)    ($v0 = lui 0x1f80, $s1 = 1 from the prologue)  -> *0x1F80019A = 1
//   0x80106E0C  sb $zero,0x19a($v0)  (phase2 teardown)                              -> *0x1F80019A = 0
// and that `sb 1` is the LAST store in phase0, after the cooperative area load `jal 0x80044BD4` and all
// eight reinit calls (0x8007b18c/0x800796dc/0x800263e8/0x80072a78/0x80075240/0x800783dc/0x80078610/
// 0x80079464). So the byte reads 1 exactly when this item's world has finished being built. DEMO's stage
// prologue (Demo::stageMain) zeroes it too.
//
// WHY `== 1` AND NOT `!= 0` — the byte is STAGE-SCOPED and shared, which is the trap here. GAME's stage
// prologue 0x8010637C writes **2** (Engine::stagePrologue), which is what cine_bars.cpp reads as `!= 2`
// and music_coord.cpp names kSpAudioState; DEMO's prologue writes **0**. renderAttract is reachable ONLY
// from the DEMO front-end path (render_walk.cpp:485, s48==7), where the byte only ever holds 0 or 1 — so
// `== 1` is a sound discriminator HERE and must not be copied to a GAME-path producer.
// Scope note, stated rather than overclaimed: the two writers above are the ones verified in the s7 phase
// machine itself. This code does NOT rest on a claim that nothing else in the whole s7 call graph writes
// the byte — it rests on the stage scoping plus the measured behaviour below (the latch reads 1 on every
// live frame and 0 on exactly the two launch frames per cycle, 7208 vs 16 frames).
//
// WHY NOT the two things that look like the gate:
//   · sm[0x4a]==1 (phase1) is NOT sufficient — phase0 advances the phase counter BEFORE the load
//     completes, so the frame after the launch reads phase==1 with no world (MEASURED, `attract` channel:
//     f465 and f1826 both read phase=1 itemBuilt=0 — and f1826 is the frame whose head0.link is the
//     faulting 0x07035D40, so phase1 alone would still have walked it).
//   · the engine-live latch 0x800BE258==2 is NOT a gate at all — it is STICKY: docs/tomba2-scene-state.md
//     line 25 records it as "set once, sticky" with a SINGLE writer (PC 0x80075374), so once the first
//     attract cycle latches it, it reads 2 for every later frame including every launch frame. Not
//     separately measured on the faulting frame — the stickiness is what disqualifies it, and that is
//     RE'd, not inferred. The file banner used to cite it as this producer's precondition.
bool Render::attractItemLive() const {
  return mCore->mem_r8(kAttractItemBuiltLatch) == 1;
}

// #6 DEMO/TITLE ATTRACT (stage 0x801062E4, sm[0x48]==7): the live 3D field WORLD under the DEMO stage —
// terrain + entity/scene tables + objects + backdrop, real per-pixel depth, exactly like the walkable
// field. renderTitle sets fps60.mTier1EligibleCur = FALSE at its top, so re-set it TRUE here (a real 3D
// world → tier-1-eligible → 60fps). The 2D 'DEMO' watermark is a deferred 2D layer (see file banner).
void Render::renderAttract() {
  Core* c = mCore;
  const bool live = attractItemLive();
  // A frame this producer DECLINED to draw must be as visible as one it drew, or "the attract world is
  // missing" and "there is no attract world yet" read identically in a log. So the line carries the
  // decision AND the state behind it: the latch, the phase counter, the item selector, and the three list
  // heads with head[0]'s link word — the exact bytes that make the launch-frame list dead.
  const uint32_t h0 = c->mem_r32(0x800FB168u);
  const bool h0InRam = h0 >= 0x80010000u && h0 < 0x80200000u;
  lucent::debug("attract", "f{} world={} itemBuilt={} phase={} item={} heads={:08X}/{:08X}/{:08X} "
                           "head0.link={:08X} (head0 in RAM={})",
                c->game->gpu.s_frame, live ? "DRAWN" : "SKIPPED(item not built)",
                c->mem_r8(kAttractItemBuiltLatch), c->mem_r16(0x801FE04Au), c->mem_r8(0x800bf870u),
                h0, c->mem_r32(0x800F2624u), c->mem_r32(0x800F2738u),
                h0InRam ? c->mem_r32(h0 + 0x24u) : 0u, h0InRam ? 1 : 0);
  // LAUNCH / TEARDOWN frames: the item's area is still streaming in (or is gone), so there is no world to
  // build a picture from — every world structure sceneNative reads (entity lists, terrain nodes, scene
  // table, backdrop struct) belongs to the item being torn down or the one not yet loaded. Draw nothing.
  // fps60 tier-1 eligibility stays FALSE (set by renderTitle at entry), so the present-time interp pass
  // does not re-run this walk either.
  //
  // NOT BLANKING is deliberate: emitting nothing holds the previously presented frame, which is the
  // conservative choice for 2 frames out of each ~1361-frame attract cycle (MEASURED: 16 declined vs 7208
  // drawn over 10890 frames). HONEST STATUS OF THIS SUB-DECISION — it is NOT verified against the
  // reference. An earlier draft of this comment claimed a PSXPORT_ORACLE measurement showing the title
  // image persisting across the launch frames; that run was never reproduced here, so the claim is removed
  // rather than repeated. If the reference turns out to blank instead, the correct change is a
  // gpu_blank_display() on this path — it cannot reintroduce the crash either way, since the crash was the
  // WALK, not the presentation. Open question, not a hidden assumption.
  if (!live) return;
  c->game->fps60.mTier1EligibleCur = true;   // native world render runs → fps60 tier-1 may re-render it
  DisplayPassGuard displayPass(c->rsub.mode);   // read-only invariant: aborts on any guest write
  sceneNative();
  cineBarsRender();     // cinematic letterbox bars (emits nothing when no cutscene bars are active)
}
