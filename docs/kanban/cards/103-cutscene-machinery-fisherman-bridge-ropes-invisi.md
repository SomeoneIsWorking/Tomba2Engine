---
id: 103
title: Cutscene machinery + fisherman + bridge ropes invisible under pc_render (area 0, live)
status: doing
labels: [render, bug]
created: 2026-08-19
updated: 2026-08-20
---

USER 2026-08-19, live windowed run (pc_faithful + pc_render), area 0, pad frame ~30150 of replays/bugs/machinery-invisible.pad (cut from the live session with padrec save). A cutscene where an NPC works a piece of MACHINERY: the machinery is INVISIBLE, the FISHERMAN the user expects in the scene is absent, and the user also reports the BRIDGE ROPES missing.

Evidence: scratch/screenshots/live/mach_now.png (live shot) and scratch/screenshots/mach/pc_30150.png (the replay reproducing it bit-for-bit on the default leg — determinism confirmed, so this is a headless repro).

Repro: PSXPORT_PAD_RESUME=replays/bugs/machinery-invisible.pad ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE  (fast-forwards ~4 min to the scene, then hands over; add PSXPORT_DEBUG_SERVER=<port> to drive it).

NOT comparable against PSXPORT_ORACLE=1 by replaying the same pad: measured 2026-08-19, the oracle leg needs 56370 native frames to reach pad frame 30150 where the default leg needs 31050, so the recorded presses land at different moments and the oracle leg ended up on a save prompt instead of the cutscene (scratch/screenshots/mach/oracle_30150.png). Use the live renderpath switch on ONE running game instead.

SUSPECTED FAMILY, not yet confirmed: the rope/tether producers. #56 (no line-primitive producer) covered ropes/fishing line; #95 (cliff fisherman body+rod absent) root-caused to GuestQueueDispatch::guestFlushesMesh answering from the jump-table ARM alone; #97 (tether producer dispatched by TYPE byte with no queue/head gate) is still open. A missing NPC + missing machinery + missing bridge ropes in one scene is the shape of ONE shared producer gap, like #56 was.

**2026-08-19:** 2026-08-19: the machinery is NOT drawn through the generic GT3/GT4 emitter. Widening the per-object redirect to func_800803DC (perobj_dispatch.cpp, the shape framework doc generic-object-producer.md describes) was verified non-regressive — A/B at a FIXED frame, AUTO_SKIP free-roam f400, 0 of 76800 px differ, with the redirect channel confirming it fired on the widened leg and not on the base leg — but the cutscene still renders without the machine (scratch/screenshots/mach/gen_cut_crop.png). So its submitter is one of the other 26 unowned rows.

NEXT STEP, and it is blocked on a tool gap: the packet->submitter lookup is 'otattr', which is REPL-ONLY (repl.cpp), and the REPL blocks the frame loop so it cannot attach to a live or long-resumed session. Put otattr on the DEBUG SERVER (same gap renderpath had, same fix), then: renderpath psx -> provat a machinery pixel -> read the packet address -> otattr that address -> the span's fn IS the submitter. Then own that submitter.

Do NOT re-derive the scene: PSXPORT_PAD_RESUME=replays/bugs/machinery-cutscene.pad reaches it in ~5 min.

**2026-08-19:** 2026-08-19 ROOT-CAUSED, with the packet attribution rather than by reasoning.

The machinery is drawn by OverlayGt3Gt4::gt4 (0x801467BC) — an emitter the per-object REDIRECT ALREADY COVERS. It is missing because the redirect is wired at ONE caller (Render::cmdListDispatch's per-mode resolution) and this object reaches the emitter from callers that have none:
  node 0x800FE408  caller 0x80136748  handler beh_event_record_machine   <- THE MACHINE ITSELF
  node 0x800F0F44  caller 0x8003CCA4  Render::perObjRenderDispatch (native-owned, no redirect)
Method (reproduce in 5 min): PSXPORT_PAD_RESUME=replays/bugs/machinery-cutscene.pad, then renderpath psx ; step 2 ; provat 125 95 -> node=<packet> ; otattr <packet>. The span carries fn/caller/node. Take the attribution IMMEDIATELY after a step — the span table is per-frame and reads back 0 spans once the frame turns over.

THE TRAP THIS SCENE SET, worth remembering: PSXPORT_DEBUG=redirect showed the redirect firing 2560+ times IN THIS CUTSCENE, so every 'is the native path running here' check answered yes while the object stayed invisible. It was firing for a different node the whole time. A per-caller count is not evidence of coverage.

Scene state is NOT the cause and was checked: sm[0x4C]=2, so render_field_native_active's authored-sub-scene bail (==3) is not taken here.

NEXT STEP: give Render::perObjRenderDispatch the same redirect cmdListDispatch has (it is already native-owned, and it carries the object's cmd, so projComposeObject + gt3gt4 apply unchanged), then RE 0x80136748 inside beh_event_record_machine for the second path. Framework guidance: external/psxport/docs/generic-object-producer.md section 3a.

Widening the redirect to the generic emitter (committed 58800be) did NOT fix this and was never claimed to — it owns the biggest shared emitter, this object does not go through it.

**2026-08-19:** **2026-08-19 — THE PREVIOUS ROOT CAUSE ON THIS CARD IS REFUTED. Measured, not reasoned.**

Two independent measurements kill it:

(1) THE ATTRIBUTION DOES NOT MEAN WHAT THIS CARD SAID IT MEANT. 'caller 0x80136748' / 'caller 0x8003CCA4' was read as 'the object reaches the emitter from callers the redirect does not cover'. It cannot support that. otattr's caller is one frame below the top of InterpDiag's otattr SHADOW STACK, and that stack is pushed by the GENERATED WRAPPER only (generated/*_disp.c ov_a00_func_X / func_X). overlay_router.cpp rec_dispatch checks 'if (overrides::dispatch(c, addr)) return;' BEFORE the wrapper, so every NATIVE-OWNED frame is INVISIBLE on that stack — and perObjRenderDispatch, cmdListDispatch, perModeDispatch and OverlayGt3Gt4::submitBlock are all native-owned. So the chain 0x80136748 -> CCA4 -> CDD8 -> F698 -> 80146478 -> 801467BC reads back as exactly 'fn=801467BC caller=80136748', with everything between erased. The attribution is CONSISTENT with the redirect path, not evidence against it.
  Also: ov_a00_gen_80136748's own FIRST act is rec_dispatch(c, 0x8003CCA4) with a0=node (generated/ov_a00_shard_0.c:20083). The 'second path with no redirect' IS perObjRenderDispatch. And every one of perObjRenderDispatch's six cases funnels through cmdListDispatch (perobj_billboard.cpp:288) — which is where the redirect already lives. There was never a second uncovered caller to wire.

(2) THE REDIRECT HAS 100% COVERAGE OF THE CMD-LIST PATH. New per-frame census, 'debug redirdiag' (game/render/perobj_dispatch.cpp), counts the DECISION with denominators and prints even when nothing fires. Over 11,782 frames of the machinery-cutscene resume, 960,390 cmds:
    drew 191,981 + already_covered 749,749 + gate_off 18,660 = 960,390 = 100.00%
    emitter_other = 0   (not one cmd resolved to an emitter the redirect does not recognise)
So no cmd on this path falls through unowned, in ANY scene of that session.

CONCLUSION: either the machinery never reaches cmdListDispatch at all, or it reaches it and the native draw emits NOTHING. 'drew' counts decisions, not prims — a draw that pushes zero prims counts as coverage while the object stays invisible, which is the same class of lie 'redirect fired 2560 times' was. Census extended to count PRIMS pushed per redirect draw and to count draws that pushed ZERO (with an example node). Re-running now.

Do NOT re-wire a redirect onto perObjRenderDispatch — it is downstream-covered already and the change would double-draw.

**2026-08-19:** **2026-08-19 — THE ACTUAL MECHANISM, measured then traced. The machinery is not on the cmd-list path at all.**

Corrected census (game/render/perobj_dispatch.cpp, 'debug redirdiag'), full 31,227-frame resume of replays/bugs/machinery-cutscene.pad:
    cmds 2,615,850 = drew 523,073 + already_covered 2,074,117 + gate_off 18,660   (100.00%)
    emitter_other = 0
    redirect draws emitted 3,934,490 prims, avg 7.52/draw, ZERO empty draws
    gate_off occurs ONLY in frames 52..2330 (narration + sub-scene). In the cutscene the gate is ON.
So every object on the per-object cmd-list path is drawn natively. The machine's mesh is FINE. What is missing is a SECOND draw the same object makes.

THE PATH: beh_event_record_machine (0x80136954) -> 0x80136748, whose gen body (generated/ov_a00_shard_0.c:20083) does exactly two things:
   1. rec_dispatch(0x8003CCA4)  — the mesh, natively covered, verified above
   2. compose a BILLBOARD transform into the GTE, then call 0x801365C4  — NOT covered, no native producer
0x801365C4 is row 5 of the global work list (87,224 guest prims across 29,085 frames) in docs/unported-render-inventory.md.

WHAT 0x801365C4 DRAWS: a vertical STACK of textured quads. It divides (int16)a1 by 120 (cpu_div -> r18 quotient, r19 remainder), loops r18 times calling 0x8003B320 and stepping Y by 120 each iteration, then emits one final partial quad of the remainder. Packet template on its own stack: code byte 0x2D (POLY_FT4), colour (128,128,128) neutral, UVs from node+96..108. That is a ROPE / CABLE / CHAIN — a tiled strip. It is very likely the SAME producer the USER's missing bridge ropes need.

WHY IT DRAWS NOTHING NATIVELY, and it is already written down in the code: game/render/quad_rtpt_submit.cpp's submitQuad (0x8003B320) is a PURE SUBSTRATE MIRROR — it fills the guest packet and OT-links it and pushes ZERO native prims. Its own banner says so: 'NO pc_render PICTURE FROM THIS LEAF', and names this exact debt — 'the a00-overlay flame/rope emitter around 0x801341xx, and the case-188 particles — STILL no native producer and no pc_render picture: honestly missing layers, tracked as portmap debt render-producer-submitquad-classes'. 0x80134064 (row 3, 626,948 prims) is that 0x801341xx family. This card is an instance of that debt, not a new bug.

WHY IT IS NOT ONE GLOBAL FIX AT 0x8003B320: 11 distinct guest fns call it (0x80114E64, 0x80120B0C, 0x8011AB3C, 0x801175C0, 0x8011A9CC, 0x8011F014, 0x80134064, 0x801365C4, 0x8013DBC0, 0x8012A2D8, 0x80114E50). Each composes its own CR0-7 contract before calling, so there is no single transform to project by, and reading the transform back out of the GTE control registers is BANNED (USER 2026-08-04: 'never do this please NEVER'). The sound global shape is: submitQuad pushes natively when the CALLER has DECLARED an active native projection (the projComposeObject/projSetActive mechanism the per-object redirect already uses), and COUNTS + REPORTS the calls that arrive with no declaration — never silently drops them. Then each caller class is a small RE that declares its transform. 11 callers, one mechanism.

0x801365C4's transform is fully RE'd from its caller and needs NO GTE tap. In 0x80136748:
    rec = mem32(node+200); WORLD_POS(0x1F8000C0) = (mem16(rec+44), mem16(rec+48), mem16(rec+52)+46)
    0x80084470(a0=0x1F8000F8 CAM_ROT, a1=0x1F8000C0 WORLD_POS, a2=0x1F800014)   -> view-space position
    matrix.t at 0x1F800014/18/1C += CAM_TRANS (0x1F80010C/110/114)
    0x80084660(a0=0x1F8000F8)  -> CR0-4 = the PURE CAMERA rotation (no object rotation: screen-aligned)
    0x80084690(a0=0x1F800000)  -> CR5-7 = that composed translation
    a1 to 0x801365C4 = (int16)mem16(node+186) — the strip LENGTH that the /120 tiling divides
So the producer needs only the node's own world state + the scene camera the native renderer already holds. Same situation FUN_8003B704's beams were in, and those were solved this way (Render::beamQuadRender, game/render/fx_beam.cpp) — that is the precedent to copy.

INSTRUMENT NOTE — one lied and was caught. The first prim census differenced RenderQueue::n across the draw and reported 'prims=-51 ... EMPTY=1'. RenderQueue::push() lazily resets n=0 on the first push of a queue frame, so any draw straddling that reset reads negative and would have been reported as 'this draw emitted nothing'. Fixed by adding RenderQueue::pushed_total, a monotonic odometer that is never reset (external/psxport/runtime/recomp/render_queue.h/.cpp). The negative values are also the proof the expression tracks reality rather than being stuck at a constant.

**2026-08-20:** 2026-08-20 — THE ROPE PRODUCER WAS DISABLED THIS WHOLE TIME, and re-enabling it does NOT fix the machinery. Both halves matter.

MY OWN LEFTOVER. game_tomba2.cpp carried:
    void fx_rope_strip_install();
    // DISABLED FOR BISECT
    // fx_rope_strip_install();
I commented it out for a bisect on 2026-08-20 and never restored it, so fx_rope_strip.cpp — the FUN_801365C4 producer written FOR this card — has never actually run. Restored.

IT FIRES, AND IT EMITS ON SCREEN. PSXPORT_DEBUG=ropefx over the machinery-cutscene resume: 1,077 emissions, each 'emitted=3 behind=0', screen=[111.1,83.3]..[118.8,126.8] — inside the 320x240 view, nothing culled behind the camera.

AND THE MACHINERY IS STILL INVISIBLE. Screenshot at f31100 of the resume (scratch/screenshots/present_31100.ppm, composite scratch/screenshots/mach_rope_ab.png) shows the same scene as the card's recorded pc_30150.png with the machine still absent. So 0x801365C4 is NOT the machinery's submitter — it is the rope/cable strip, and it is now produced. The machinery remains one of the other unowned rows, exactly as the card's earlier analysis said.

DO NOT READ THE COMPOSITE AS A PIXEL A/B. The two shots are 950 frames apart (30150 vs 31100), so characters have moved and a pixel diff between them is meaningless. It is a VISUAL check of one thing only: is the machine there. It is not.

A DIAGNOSTIC BUG OF MINE, FIXED: the ropefx line hardcoded 'f0' instead of the frame, so 1,077 lines all claimed frame 0 and there was no way to tell WHEN the producer ran — whether the emissions were the bridge ropes early in area 0 or the cutscene ropes. It now prints gpu_frame_no(c). That matters for the next step, because 'it emits' and 'it emits IN THIS SCENE' are different claims and the log could not separate them.

ALSO FIXED, per the USER's never-duplicate rule: gpu_frame_no was declared THREE times — once properly in the framework's gpu_native_internal.h and again as a local `extern int gpu_frame_no(Core*);` in both fx_rope_strip.cpp and perobj_dispatch.cpp. Both local re-declarations deleted, header included instead.

THE WORKFLOW COST, measured, because the card understates it: this repro is documented as '~4.7 min at ~110 fps'. It actually took ~25 MINUTES of wall clock, and the ropefx channel itself roughly halved the rate (1,000s of formatted lines). That is the same defect the USER called out on 2026-08-20 ('this ~5 mins to reproduce is totally unacceptable'), and it is worse than recorded. A savestate/scene-warp for this scene would pay for itself immediately.

NEXT: with the frame number now in the log, re-run and check whether ropefx fires DURING the cutscene frames (~31,000+) or only earlier. If it fires there and the ropes are still not visible, the producer emits into a queue whose prims are not presented — which is #98's ledger question, not an RE question.

**2026-08-20:** 2026-08-20 (follow-up) — WITH FRAME NUMBERS IN THE LOG, the producer's firing pattern is now readable, and it settles half the card.

    first emission   f1239
    still emitting   f11403 (continuously, tracking the frame counter)
So fx_rope_strip is NOT a one-off: it produces the rope/cable strip throughout area 0, from early free-roam onward. The BRIDGE ROPES half of this card is therefore produced now; the MACHINERY half is not, and they were always two different submitters sharing one card.

A CLEAN NEGATIVE, worth recording so nobody re-runs it: the producer does NOT fire at all in the first 600 frames of replays/bugs/seesaw-weight.pad. That is correct (no rope strip in that scene) and it is why a cheap short replay cannot substitute for the area-0 resume when testing this producer.

WHAT IS LEFT ON THE MACHINERY, and it is no longer an RE question about 0x801365C4:
  * 0x80136748 — the caller that composes the transform and then calls BOTH 0x8003CCA4 (the mesh) and 0x801365C4 (the rope) — has NO NATIVE OWNER (codemap --addr 80136748, checked against 1033 indexed natives / 474 install sites; the tool prints its own blind spots and none of them apply here).
  * The mesh path through 0x8003CCA4 IS covered by the per-object redirect — the redirdiag census on this card measured 100.00% coverage of the cmd-list path with zero empty draws.
So the machinery's prims are produced and the redirect claims them, yet nothing appears. That makes this a PRODUCED-VS-PRESENTED question (#98), not a missing-producer one: either the prims never reach the screen, or they reach it with a transform/depth that puts them outside the view. Next measurement is the ledger at the cutscene frame, not more RE.
