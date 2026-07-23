---
id: 45
title: CAMPAIGN: render everything natively — retire the substrate-GTE projection producers to float-native
status: doing
labels: [render, campaign]
created: 2026-07-23
updated: 2026-07-23
---

USER 2026-07-23: 'I'd rather have everything rendered natively.' The umbrella card for converting every producer whose PICTURE still comes from the substrate (a gen-body scrape or a Beetle-GTE gte_op projection) into a float-native producer that projects with the native camera. Two payoffs: it is what the user wants, and a float-native producer can be re-run one frame behind under a lerped camera, so it LERPS at fps60 by construction (a GTE/scrape producer cannot — re-running it would drive/read the guest GTE or write guest RAM). This closes the fps60 REDIRECT backlog (#31/#16/#23) as a side effect.

CLASSIFICATION (built by grep audit 2026-07-23, not guessed — the tells are: gte_read_data = scrapes the RTPS RESULT; 0x1F8000xx read = scrapes the projection scratchpad; gte_op( = drives the Beetle GTE to project; vs proj_native/EObjXform/drawWorldQuad + template-table/2D reads = already float-native):

TIER A — TRUE TAP (scrapes the gen body's published projection):
  - game/render/fx_sprite.cpp (flame, FUN_80027A4C family) — BEING PORTED now (worktree agent, the reference implementation for this campaign). node+0x2C/+0x30 world anchor projected natively; plug into fieldObjectsRender type-0x20 branch like narration_swirl.

TIER B — GTE-DRIVING TRANSCRIPTIONS (project via gte_op, from game state but PSX-mechanism; the render directive already bans 'no gte_op for render'; cannot lerp):
  - game/render/overlay_gt3gt4.cpp (22 gte_op)  — A00-overlay GT3/GT4 packet emitters (0x801465EC/801467BC)
  - game/render/overlay_ground_gt3gt4.cpp (21)  — A00 ground/scene GT3/GT4 + entity loop (0x8013FB88/FE58/401B8)
  - game/render/perobj_billboard.cpp (15)       — billboard composer
  - game/render/quad_rtpt_submit.cpp (10)       — rope/flame quad RTPT (0x8003B054/8003B320)
  - game/render/perobj_dispatch.cpp (9)         — per-object render dispatch
  - game/render/widescreen_margin_quad.cpp (8)  — widescreen margin GT4

TIER C — ALREADY FLOAT-NATIVE, DO NOT TOUCH (picture from game data / native float matrices; the gen_func they run is for guest-STATE fidelity / SBS byte-exactness, which is REQUIRED, not a rendering shortcut — running the guest body is not the same as scraping it for the picture):
  - native_terrain, swing_fx (charge), fx_mesh (impact), field_hud, score_popup, screen_fade, and the UI emitters emitUiFt4/emitUiSprites/panel/panel_fill (template-DATA driven, zero GTE).

METHOD per producer (the fx_sprite port is the template once it lands): RE the transform the gte_op path composes, reproduce it with proj_native_vertex/projComposeCamera (0-diff vs Beetle, docs tomba2-native-projection), read the camera from Fps60::sceneCam (lerped at the interp present), emit with real per-vertex depth + has_xyf set so it is tier1-owned and lerps. Gate each: pixel-match vs the pre-port GTE output at a fixed frame, fps60dump lerp proof, SBS still 0-diff (guest still runs its gen body). DO NOT fan out until the fx_sprite reference port is verified — porting GTE->float before the first is proven risks the same 'verified in isolation, collides on merge' failure just hit integrating the four producers.

ORDER: A (flame, in progress) proves the pattern -> then B one at a time or in a small fleet with the operator integrating, world-effect emitters (quad_rtpt = rope flame, #23's sibling) before overlay/terrain-class (bigger, geometry). Track each B producer's flip to float-native here.

**2026-07-23:** 2026-07-23 TIER A DONE — the fx_sprite flame reference port landed (commit 4ba2e5e), all 5 gates pass incl. the lerp gate (interp flame at midpoint of real neighbours 275.77->276.22 interp 276.20; SBS 0-diff to f4680; no gen_func for the picture; the tap install on 0x80027A4C deleted). The type-0x20 fieldObjectsRender dispatch alongside narration_swirl is the proven template for Tier B: RE the emitter's transform, project via projComposeCamera(sceneCam) + EObjXform, emit drawWorldQuad with has_xyf=1 so tier1 re-renders it lerped. Bonus: it now renders under GATE=1 too (native producer runs on every exec leg), retiring #12's 'taps don't fire under GATE' limitation. NEXT: Tier B, quad_rtpt_submit (rope flame, the direct sibling) first.

**2026-07-23:** 2026-07-23 TIER B WAS A MISCLASSIFICATION — campaign re-scoped and largely COMPLETE. The first Tier-B port attempt (quad_rtpt_submit) resolved on 'already float-native, do not port', which exposed that grepping for gte_op( conflated two unrelated GTE uses (see docs/findings/render.md 'gte_op count does NOT mean not native'):
  (1) FAITHFUL GUEST-STATE MIRRORS — overlay_gt3gt4, overlay_ground_gt3gt4, perobj_dispatch, rotateQuadCorners: drive the GTE to bump-copy byte-exact packets into the guest pool+OT for SBS. overlay_gt3gt4's own header: 'faithful substrate mirror, NOT pc_render ... every GTE op is REQUIRED, SBS gates it.' These are Job #1 faithful execution, NOT taps — retiring them would BREAK SBS. They are NOT campaign targets. Keep.
  (2) ALREADY FLOAT-NATIVE PICTURE PRODUCERS — quad_rtpt_submit(#67), perobj_billboard, widescreen_margin_quad: record model corners + factored world transform into Render::mWqRecs/mBbRecs; Render::billboardsRender (re-run by tier1Render from fieldObjectsRender) re-projects through the float lerped sceneCam and component-lerps vs *Prev. No gte_op in the picture path; they lerp. Already Tier C. DONE.
The ONLY true scrape-tap in the whole render tree was fx_sprite (the flame), now ported (Tier A). So the 'retire the substrate-GTE producers' framing is CLOSED — there was nothing to retire beyond the flame. What remains of 'render everything natively' is the MISSING-PRODUCER gaps where geometry has NO native producer and is invisible: #39 (dust), #42 (sky/background planes), #43 (minimaps), #44 (vortex). Those are the real remaining work — tracked on their own cards. Re-pointing this campaign at them. Classification method for any future producer is recorded in the finding: grep mWqRecs/mBbRecs/billboardsRender + the header's substrate-mirror claim BEFORE calling anything a port target; a gte_op count alone proves nothing.

**2026-07-23:** 2026-07-23 MISSING-PRODUCER progress — #42 (sky/background planes) resolved for the tilemap family. The shared area-backdrop tilemap producer now covers seaside + areas 10 and 11 (69761->18022 and 45719->26580 px >8/255 vs a real psx reference), via Render::backdropTilemapDrawer resolving the guest's own backdrop dispatch (gen_func_8003DF04) to the RESIDENT overlay drawer and reading that drawer's baked V bias from its own code instead of the old scene heuristic. Native producer, no gte_op, read-only, SBS 0-diff to f16770, seaside 0-diff before/after.

The remaining #42 areas turned out NOT to share the cause and were split out: #47 (area 14 — the waterfall is GTE scene geometry; its bg-state indexes the guest's 'draw nothing' jump-table entry and PARALLAX_BG_SM is zeroed) and #48 (area 21 — gouraud-gradient base + tilemap cloud overlay composite; the tilemap alone measured WORSE so it is excluded until the gradient producer lands). A separate effect gap also surfaced: area 11's night-sky STAR sparkles are their own missing effect producer, not part of the backdrop.

Campaign remainder is therefore: #39 (dust), #43 (minimaps), #44 (vortex), #47 (a14 waterfall geometry), #48 (a21 gradient sky).
