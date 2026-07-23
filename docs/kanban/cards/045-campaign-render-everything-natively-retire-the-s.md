---
id: 45
title: CAMPAIGN: render everything natively — retire the substrate-GTE projection producers to float-native
status: todo
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
