# Code map — guest address → PC-native owner

> EMITTED by `tools/codemap.py` — do not edit by hand; rerun the tool.

Before reimplementing any `FUN_xxxx`, look it up here (or `tools/codemap.py --addr <hex>`).
A native may exist already. **LIVE** = reachable by a real call from either a native_boot
dispatch root or ordinary (non-native-tagged) game/engine code — free-function syntax
(`ov_foo(...)`), qualified static syntax (`Class::method(...)`), or C++ instance-call
syntax (`obj.method(...)`, `ptr->method(...)`, bare in-class `method(...)`). **ORPHAN** =
native exists but no call site of any of those forms was found anywhere in the tree — it
is genuinely dead code until something calls it.

Totals: 800 native fns, 642 owned addresses, 793 LIVE / 7 ORPHAN. 241 override declaration sites over 241 addresses.

**A row can come from a DEFINITION or from an INSTALL SITE.** An address whose handler is a file-local static in an anonymous namespace (no address in its name, no tag, no quoted registry name) has no findable definition — the `tomba::native::declareOverride` / `tomba::native::declareOverride*` call site is its only ownership record, and the file holding that call site is where you debug it from. Those rows say so in the summary column.

**Cross-check `docs/port-map.md` before porting anything.** This map answers WHERE code lives; it cannot answer whether a layer should exist. Layers whose producer was DELETED ON PURPOSE (the no-tap rule) look identical here to unported ones. `--addr <hex>` performs the cross-reference; the section at the end of this file lists every deliberately-absent step.

| addr | status | symbol | file:line | depends-on (still-PSX) | summary |
|------|--------|--------|-----------|------------------------|---------|
| 0x8001CAC0 | LIVE | `Engine::areaModeDispatchFaithful` | game/core/engine.cpp:3137 | 0x8001CB98 | Engine::areaModeDispatch — the 22-way area-mode dispatcher at guest |
| 0x8001D364 | LIVE | `AudioDispatch::voiceFetchBits` | game/audio/audio_dispatch.cpp:54 | 0x8001D2A8 | AudioDispatch::voiceFetchBits — native ownership of FUN_8001D364 (Ghid… |
| 0x8001D71C | LIVE | `AudioDispatch::zoneTransitionSetup` | game/audio/audio_dispatch.cpp:111 | 0x8001CF2C 0x8001D2A8 | AudioDispatch::zoneTransitionSetup — native ownership of the tiny disp… |
| 0x8001F40C | LIVE | `CollisionResolve::classifyBodyContact` | game/world/collision_resolve.cpp:604 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x8001F9DC | LIVE | `MeleeProximity::isAtApproachAnchor` | game/ai/melee_proximity.cpp:17 | 0x80084080 |  |
| 0x8001F9DC | LIVE | `MeleeProximity::isAtApproachAnchorFramed` | game/ai/melee_proximity.cpp:66 |  |  |
| 0x8001F9DC | LIVE | `MeleeProximity::registerOverrides` | game/ai/melee_proximity.cpp:103 |  |  |
| 0x8001FAE0 | LIVE | `ActorTargeting::tryAcquireTarget` | game/player/actor_targeting.cpp:97 |  | ORACLE: guest 0x8001FAE0 |
| 0x80020364 | LIVE | `ActorTomba::stepModeInteract` | game/player/actor_tomba.cpp:693 |  | postInteractWalk case 0xF/0x14/0x56 (mode=0) / 0x2F (mode=2). |
| 0x800205CC | LIVE | `ActorTomba::type8Interact` | game/player/actor_tomba.cpp:815 |  | postInteractWalk case 8. |
| 0x80022060 | LIVE | `ActorTomba::proximityCheck` | game/player/actor_tomba.cpp:338 |  | cylinder proximity + Y-band check. |
| 0x80022190 | LIVE | `ActorTomba::subHitboxCheck` | game/player/actor_tomba.cpp:393 |  | per-sub-hitbox collision variant. |
| 0x80022760 | LIVE | `ActorTomba::interactWalk` | game/player/actor_tomba.cpp:271 |  | ======================================================================… |
| 0x80022A80 | LIVE | `Engine::modePerFrameDispatchFaithful` | game/core/engine.cpp:3433 |  | Engine::modePerFrameDispatchFaithful — pc_faithful mirror of |
| 0x80022C78 | LIVE | `ActorTomba::growthYSnap` | game/player/actor_tomba.cpp:893 |  | leaf, no guest-stack frame. Operates on G (postFrameWaterCheck's |
| 0x800235A0 | LIVE | `ActorTomba::type7Interact` | game/player/actor_tomba.cpp:870 |  | postInteractWalk case 7. |
| 0x80023A04 | LIVE | `CollisionResolve::resolveByContactPolicy` | game/world/collision_resolve.cpp:794 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80023D48 | LIVE | `CollisionResolve::cylinderResolve` | game/world/collision_resolve.cpp:290 |  | ORACLE: guest 0x80023D48 |
| 0x8002423C | LIVE | `CollisionResolve::landOnObjectTop` | game/world/collision_resolve.cpp:497 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80024794 | LIVE | `interact_scan` | game/player/interact_scan.cpp:71 |  | (player) -> 1 if something was activated this call, else 0. |
| 0x80025588 | LIVE | `Engine::sceneEventFifo` | game/core/engine.cpp:696 |  | Native FUN_80025588 — the field EVENT/COMMAND-QUEUE state machine (str… |
| 0x80025588 | LIVE | `Engine::sceneEventFifoFaithful` | game/core/engine.cpp:777 |  | pc_faithful field EVENT/COMMAND-QUEUE state machine — mirror of |
| 0x80025744 | LIVE | `Render::fieldHudStatusRow` | game/render/field_hud.cpp:393 |  | --- FUN_80025744 — status row ----------------------------------------… |
| 0x80025934 | LIVE | `Render::fieldHudItemRing` | game/render/field_hud.cpp:425 |  | --- FUN_80025934 — item ring -----------------------------------------… |
| 0x80025B78 | LIVE | `Render::fieldHudWeaponStrip` | game/render/field_hud.cpp:492 |  | --- FUN_80025B78 — equipped-weapon strip (the kanban #13 layer) ------… |
| 0x80025D98 | LIVE | `Render::fieldHudRender` | game/render/field_hud.cpp:532 |  | --- FUN_80025D98 — the HUD dispatcher gate (transcribed 1:1) ---------… |
| 0x80026368 | LIVE | `Array8Dispatch::tickFaithful` | game/object/array8_dispatch.cpp:25 |  | tickFaithful(): line-for-line mirror of guest 0x80026368 (authenticate… |
| 0x800263E8 | LIVE | `Pool::seedAreaObjects` | game/world/pool.cpp:173 |  | area object-record seeding. Selects a per-area byte sequence (table 0x… |
| 0x80026470 | LIVE | `BgSceneTransitionSm::midTransitionGate` | game/scene/bg_scene_transition_sm.cpp:90 |  | Common guard shared by FUN_80026470/80026510/800264BC — three inline a… |
| 0x80026470 | LIVE | `BgSceneTransitionSm::audioStub26470` | game/scene/bg_scene_transition_sm.cpp:96 |  |  |
| 0x800264BC | LIVE | `BgSceneTransitionSm::audioStub264BC` | game/scene/bg_scene_transition_sm.cpp:106 |  |  |
| 0x80026510 | LIVE | `BgSceneTransitionSm::audioStub26510` | game/scene/bg_scene_transition_sm.cpp:101 |  |  |
| 0x8002655C | LIVE | `BgSceneTransitionSm::body` | game/scene/bg_scene_transition_sm.cpp:123 |  |  |
| 0x80026C88 | LIVE | `ObjectTable::dispatch` | game/world/object_table.cpp:139 | 0x80026C88 |  |
| 0x80026C88 | LIVE | `ObjectTable::dispatchFaithful` | game/world/object_table.cpp:218 |  | ObjectTable::dispatchFaithful — byte-mirror of guest 0x80026C88 (authe… |
| 0x80027254 | LIVE | `ObjectTable::handler27254` | game/world/object_table.cpp:43 |  |  |
| 0x80027768 | LIVE | `waterJetWriterTap` | game/render/guest_gte_water_jet.cpp:178 | 0x80027768 | untouched guest packed-mesh writer plus one scoped packet-span replay.… |
| 0x80027A4C | LIVE | `Render::fxSpriteRender` | game/render/fx_sprite.cpp:391 |  | The node's own render fn IS the emitter for every plain member of the … |
| 0x80027CB4 | LIVE | `FxSpriteAnchored::emitUniformScale` | game/render/fx_sprite_anchored.cpp:246 |  | GUEST_ADDRESS: 80027CB4 authenticated executable/overlay evidence |
| 0x80027E5C | LIVE | `FxSpriteAnchored::emitByteScale` | game/render/fx_sprite_anchored.cpp:327 |  | GUEST_ADDRESS: 80027E5C authenticated executable/overlay evidence |
| 0x800281EC | LIVE | `FxSpriteSwarm::emitPerParticle` | game/render/fx_sprite_swarm.cpp:156 |  | GUEST_ADDRESS: 800281EC authenticated executable/overlay evidence |
| 0x800286CC | LIVE | `Render::fxAnimSpriteRender` | game/render/fx_sprite.cpp:565 |  | The FUN_800286CC emitter, rebuilt: read the effect node's own animatio… |
| 0x800288AC | LIVE | `Render::impactPlumeRender` | game/render/fx_impact.cpp:54 |  | one packed-mesh copy at the impact node's anchor. The surrounding type… |
| 0x8002918C | LIVE | `beh_rand_phase_cull` | game/ai/beh_rand_phase_cull.cpp:63 |  |  |
| 0x80029664 | LIVE | `Render::dustTrailEmit` | game/render/fx_dust.cpp:116 |  | the trail: thread the ring's first four recorded positions and lay two… |
| 0x80029B40 | LIVE | `beh_pos_history_trail` | game/ai/beh_pos_history_trail.cpp:65 |  |  |
| 0x80029F6C | LIVE | `Render::dustEffectRender` | game/render/fx_dust.cpp:273 |  | the dust node's custom render fn, rebuilt natively. |
| 0x8002A834 | LIVE | `Render::swingStarburstRender` | game/render/fx_swing.cpp:47 |  | Guest FUN_8002A834's ten-copy weapon-charge starburst, rebuilt from th… |
| 0x8002AB5C | LIVE | `NativeScenePass::terrainRender` | game/render/native_terrain.cpp:103 |  |  |
| 0x8002AB5C | LIVE | `Render::terrain` | game/render/submit.cpp:789 |  | RETIRED 2026-07-07 (issue #32): Render::prepObjectMatrix (guest sway/I… |
| 0x8002AE0C | LIVE | `Render::objectHighlightRender` | game/render/object_highlight.cpp:45 |  |  |
| 0x8002B278 | LIVE | `Cull::coneCullBody` | game/render/cull.cpp:327 |  | standalone view-CONE cull (3.9% field hot). a0 = node. The multiply-fo… |
| 0x8002B278 | LIVE | `Cull::coneCull2b278` | game/render/cull.cpp:353 |  |  |
| 0x8002B3A4 | LIVE | `Render::fxSpriteEmit` | game/render/fx_sprite.cpp:411 |  | the shared sprite-family body owns this controller's ring-rotation bra… |
| 0x8002BC9C | LIVE | `Render::radialPlumeRender` | game/render/fx_plume.cpp:97 |  | The four-copy radial plume of guest FUN_8002BC9C, rebuilt from the nod… |
| 0x8002E680 | LIVE | `Render::impactAnnulusDraw` | game/render/fx_ring.cpp:121 |  | the shared annulus leaf, as a native producer. Centre and scale are al… |
| 0x8002ECD8 | LIVE | `Render::impactRingRender` | game/render/fx_ring.cpp:210 |  | the node half: resolve centre/scale/depth, animate the radii, hand off… |
| 0x8003116C | LIVE | `Spawn::spawnAndInitBody` | game/world/spawn.cpp:263 | 0x80028E10 | SPAWN-AND-INIT helper: spawn a type-6 object on list 1 (via the owned … |
| 0x8003116C | LIVE | `Spawn::spawnAndInit` | game/world/spawn.cpp:422 |  |  |
| 0x80031558 | LIVE | `Spawn::spawnEffectChild` | game/world/spawn.cpp:492 | 0x8007A980 | Spawn::spawnEffectChild. One of the near-identical MAIN.EXE "spawn a c… |
| 0x80031708 | LIVE | `ScriptInterp::refreshCachedTailHi` | game/scene/script_interp.cpp:1132 |  | ORACLE: guest 0x80031708 |
| 0x80031744 | LIVE | `ScriptInterp::refreshCachedTailLo` | game/scene/script_interp.cpp:1148 |  | ORACLE: guest 0x80031744 |
| 0x80031780 | LIVE | `Collision::listScan` | game/player/collision.cpp:247 | 0x80031780 | list-tail resolver / reset. Walks the 8-byte-stride linked list rooted… |
| 0x800318A0 | LIVE | `ObjModelView::composeIntoGte` | game/render/obj_model_view.cpp:152 |  | ORACLE: guest 0x800318A0 (tools/dynamic differential evidence equivale… |
| 0x800328EC | LIVE | `Render::altSpriteEmit` | game/render/fx_sprite.cpp:640 |  | ── FUN_800328EC family producers ─────────────────────────────────────… |
| 0x80032A44 | LIVE | `Rng::inRange` | game/math/rng.cpp:13 |  | scaled random. Disas 0x80032A44..0x80032A84 verbatim: `sra v0, 15` on … |
| 0x80033080 | LIVE | `Render::impactBurstRender` | game/render/fx_sprite.cpp:406 |  | the WEAPON-IMPACT burst (kanban #15), a COMPOSITE render fn: it is not… |
| 0x800346BC | LIVE | `PauseMenu::install` | game/ui/pause_menu.cpp:179 |  |  |
| 0x80036DFC | LIVE | `SaveMenu::runHandler` | game/ui/save_menu.cpp:103 |  | ----------------------------------------------------------------------… |
| 0x80036DFC | LIVE | `SaveMenu::dispatchBody` | game/ui/save_menu.cpp:139 |  | ----------------------------------------------------------------------… |
| 0x80039E80 | LIVE | `emitGlyph` | game/render/cube_text_banner.cpp:274 |  | The GLYPH half. UVs are FUN_80039E80's atlas lookup, reproduced: u0 = … |
| 0x80039F4C | LIVE | `ov_textLabelEmit` | game/render/text_label.cpp:171 |  |  |
| 0x80039F4C | LIVE | `Render::textLabelEmit` | game/render/text_label.cpp:176 |  |  |
| 0x8003AD48 | LIVE | `beh_cube_text_spawn` | game/ai/beh_cube_text_spawn.cpp:60 | 0x8003A790 0x8003A9A0 0x8003ABE4 0x8009A730 |  |
| 0x8003B054 | LIVE | `QuadRtptSubmit::rotateQuadCorners` | game/render/quad_rtpt_submit.cpp:43 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x8003B054 | LIVE | `QuadRtptSubmit::registerOverrides` | game/render/quad_rtpt_submit.cpp:264 |  | Wiring (frontier, 2026-07-08): both leaves are reached only via direct… |
| 0x8003B220 | ORPHAN | `hitbox_build_3b220` | game/player/hitbox.cpp:52 |  | Pure native body. Mirrors the guest instruction path's exact in-memory… |
| 0x8003B320 | LIVE | `QuadRtptSubmit::submitQuad` | game/render/quad_rtpt_submit.cpp:134 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x8003B704 | LIVE | `Render::beamNodeReached` | game/render/fx_beam.cpp:131 |  | beamNodeReached — which of FUN_8003EEC0's arms this node takes, read f… |
| 0x8003B704 | LIVE | `Render::beamQuadRender` | game/render/fx_beam.cpp:149 |  | beamQuadRender — FUN_8003B704's picture. Read-only; emits world quads … |
| 0x8003BB50 | LIVE | `Render::objListWalk1` | game/render/objlist_walk.cpp:102 | 0x8002AE0C 0x8003C5F8 0x8003C788 0x80122974 | ======================================================================… |
| 0x8003BB50 | LIVE | `ov_objListWalk1` | game/render/objlist_walk.cpp:547 |  |  |
| 0x8003BCF4 | LIVE | `Render::objListWalk2` | game/render/objlist_walk.cpp:250 |  | ======================================================================… |
| 0x8003BCF4 | LIVE | `ov_objListWalk2` | game/render/objlist_walk.cpp:550 |  |  |
| 0x8003BDAC | LIVE | `ov_objListWalk2Case0` | game/render/objlist_walk.cpp:574 | 0x8003BED8 0x8003CCA4 | jump-table case 0/15 of the object-type table at 0x80014CB0. NOT A FUN… |
| 0x8003BED8 | LIVE | `Render::objListWalk2Continue` | game/render/objlist_walk.cpp:317 |  | (Render::objListWalk2Continue) — the walk's shared "process the rest o… |
| 0x8003BED8 | LIVE | `ov_objListWalk2Continue` | game/render/objlist_walk.cpp:553 |  |  |
| 0x8003BF00 | LIVE | `Render::objListWalk3` | game/render/objlist_walk.cpp:354 | 0x8003C5F8 0x8003C788 0x8004CC88 0x8010FC70 | ======================================================================… |
| 0x8003BF00 | LIVE | `ov_objListWalk3` | game/render/objlist_walk.cpp:556 |  |  |
| 0x8003C048 | LIVE | `Render::renderWalk` | game/render/render_walk_dispatch.cpp:155 | 0x80039F4C 0x8003C5F8 0x8003C788 0x8003EF9C 0x8003F174 0x800726D4 … |  |
| 0x8003C048 | LIVE | `ov_renderWalk` | game/render/render_walk_dispatch.cpp:285 |  |  |
| 0x8003C2D4 | LIVE | `Render::billboardCompose1` | game/render/perobj_billboard.cpp:476 |  |  |
| 0x8003C2D4 | LIVE | `ov_billboardCompose1` | game/render/perobj_billboard.cpp:928 |  |  |
| 0x8003C464 | LIVE | `Render::billboardCompose2` | game/render/perobj_billboard.cpp:522 | 0x800517BC |  |
| 0x8003C464 | LIVE | `ov_billboardCompose2` | game/render/perobj_billboard.cpp:931 |  |  |
| 0x8003C5F8 | LIVE | `Render::billboardComposeC5F8` | game/render/perobj_billboard.cpp:653 |  | ======================================================================… |
| 0x8003C5F8 | LIVE | `ov_billboardComposeC5F8` | game/render/perobj_billboard.cpp:937 |  |  |
| 0x8003C788 | LIVE | `Render::billboardCompose3` | game/render/perobj_billboard.cpp:580 |  | ======================================================================… |
| 0x8003C788 | LIVE | `ov_billboardCompose3` | game/render/perobj_billboard.cpp:934 |  |  |
| 0x8003C8F4 | LIVE | `Render::billboardEmit` | game/render/perobj_billboard.cpp:702 | 0x8003B054 0x8003B220 | ======================================================================… |
| 0x8003C8F4 | LIVE | `ov_billboardEmit` | game/render/perobj_billboard.cpp:940 |  |  |
| 0x8003CCA4 | LIVE | `Render::perObjRenderDispatch` | game/render/perobj_billboard.cpp:314 |  | ======================================================================… |
| 0x8003CCA4 | LIVE | `ov_perObjRenderDispatch` | game/render/perobj_billboard.cpp:925 |  | Engine/game natives installed into the per-Core image-qualified runtim… |
| 0x8003CDD8 | LIVE | `MarginRenderer::collect` | game/render/margin_render.cpp:165 |  | Record a re-include-eligible node (deduped within the frame). FILTER t… |
| 0x8003CDD8 | LIVE | `Render::cmdListDispatch` | game/render/perobj_dispatch.cpp:208 |  | per-object cmd-list dispatch: composes the WORLD object transform (cam… |
| 0x8003CDD8 | LIVE | `ov_cmdListDispatch` | game/render/perobj_dispatch.cpp:550 |  |  |
| 0x8003D0BC | LIVE | `Render::overlayTypeDispatch` | game/render/overlay_type_dispatch.cpp:70 | 0x8010AA20 0x8010B0B8 0x8010B5BC 0x8010BA40 0x8010C2A4 0x8011024C … |  |
| 0x8003D0BC | LIVE | `ov_overlayTypeDispatch` | game/render/overlay_type_dispatch.cpp:173 |  |  |
| 0x8003D584 | LIVE | `Render::effectColorAdd` | game/render/effect_mod.cpp:208 |  | modulate each colour channel by the node's per-channel amount, rather … |
| 0x8003DF04 | LIVE | `Render::backdropTilemapDrawer` | game/render/backdrop.cpp:28 |  | ======================================================================… |
| 0x8003EEC0 | LIVE | `Render::objListWalk4` | game/render/objlist_walk.cpp:456 | 0x8003B704 | ======================================================================… |
| 0x8003EEC0 | LIVE | `ov_objListWalk4` | game/render/objlist_walk.cpp:559 |  |  |
| 0x8003EF9C | LIVE | `Render::composeTintGate` | game/render/compose_tint_gate.cpp:48 | 0x8003D584 0x8003F07C | ORACLE: guest 0x8003EF9C |
| 0x8003F07C | LIVE | `Render::sharedTransformWalk` | game/render/subpart_walk_shared.cpp:38 | 0x8003F698 | ORACLE: guest 0x8003F07C |
| 0x8003F174 | LIVE | `Render::subPartWalk` | game/render/subpart_walk.cpp:45 | 0x8003F698 | ORACLE: guest 0x8003F174 |
| 0x8003F344 | LIVE | `Render::effectClutSwap` | game/render/effect_mod.cpp:180 |  | stamp the node's CLUT id onto every colour-bearing packet, repointing … |
| 0x8003F3F4 | LIVE | `Render::effectSemiOn` | game/render/effect_mod.cpp:165 |  | turn semi-transparency ON for every colour-bearing packet in the span. |
| 0x8003F4C4 | LIVE | `Render::effectSemiOff` | game/render/effect_mod.cpp:172 |  | the exact inverse: turn semi-transparency OFF. |
| 0x8003F594 | LIVE | `Render::effectFlatTint` | game/render/effect_mod.cpp:190 |  | overwrite the packet's colour word(s) with one flat colour and force s… |
| 0x8003F698 | LIVE | `Render::resolvePerModeEmitter` | game/render/perobj_dispatch.cpp:509 |  | WHICH GUEST EMITTER a cmd with this `flag` resolves to — the ONE encod… |
| 0x8003F698 | LIVE | `Render::perModeDispatch` | game/render/perobj_dispatch.cpp:535 | 0x800803DC | per-mode render dispatcher: routes to the area's per-mode renderer (mo… |
| 0x8003F698 | LIVE | `ov_perModeDispatch` | game/render/perobj_dispatch.cpp:553 |  |  |
| 0x8003F9A8 | LIVE | `Render::frame` | game/render/render_frame.cpp:67 |  | per-frame render orchestrator. The render-queue WALK passes (0x8003bf0… |
| 0x8003FA44 | LIVE | `Render::frameX` | game/render/render_frame.cpp:79 |  | mid-transition render orchestrator twin (reduced pass set). Same rule:… |
| 0x8003FD10 | ORPHAN | `osc_fd10` | game/world/entity.cpp:46 |  | per-object OSCILLATE / FRAME-TOGGLE sub-behavior (PlacedPropSm STATE-1… |
| 0x80040558 | LIVE | `PlacedPropSm::step` | game/ai/placed_prop_sm.cpp:142 |  | ORACLE: guest 0x80040558 |
| 0x80040A58 | LIVE | `SceneEvents::classSize` | game/scene/scene_events.cpp:43 |  |  |
| 0x80040AA4 | LIVE | `CubeTextLedger::spawnPopup` | game/object/cube_text_ledger.cpp:92 |  |  |
| 0x80040B48 | LIVE | `SceneEvents::armBody` | game/scene/scene_events.cpp:71 |  |  |
| 0x80040B48 | LIVE | `SceneEvents::arm` | game/scene/scene_events.cpp:116 |  |  |
| 0x80040B48 | LIVE | `SceneEvents::armOverride` | game/scene/scene_events.cpp:127 |  | override entry (guest ABI: slot in r4, ret in r2). Single canonical bo… |
| 0x80040C00 | LIVE | `CubeTextLedger::deactivateSlot` | game/object/cube_text_ledger.cpp:64 |  |  |
| 0x80040CDC | LIVE | `ScriptInterp::init` | game/scene/script_interp.cpp:110 |  |  |
| 0x80040DE0 | LIVE | `ScriptInterp::loadCurrentEntry` | game/scene/script_interp.cpp:134 |  |  |
| 0x80040E54 | LIVE | `ScriptInterp::loadNextEntry` | game/scene/script_interp.cpp:313 |  | loadNextEntry(obj, kindArg): THE ENTRY ADVANCE. 1:1 with guest 0x80040… |
| 0x80040FA0 | LIVE | `ScriptInterp::advanceStep` | game/scene/script_interp.cpp:424 | 0x80040E54 | VERIFIED + WIRED (frontier tier, 2026-07-10; advanceEntry() now calls … |
| 0x80041098 | LIVE | `beh_script_interp_step` | game/scene/script_interp.cpp:554 |  |  |
| 0x80041098 | LIVE | `ScriptInterp::step` | game/scene/script_interp.cpp:559 |  |  |
| 0x800412CC | LIVE | `ScriptInterp::callFnptr` | game/scene/script_interp.cpp:510 |  |  |
| 0x8004139C | LIVE | `ScriptInterp::stepAngleToward` | game/scene/script_interp.cpp:679 |  | leaf angle-stepper (no guest frame). See script_interp.h for the seman… |
| 0x80041438 | LIVE | `ScriptInterp::turnFacing` | game/scene/script_interp.cpp:710 |  | thin wrapper: turnFacing(obj, targetAngle, step) = stepAngleToward(obj… |
| 0x80041438 | LIVE | `ScriptInterp::turnFacingFramed` | game/scene/script_interp.cpp:716 |  | Guest-ABI twin — mirrors FUN_80041438's own sp-=24 / ra-spill-at-+16 f… |
| 0x80041468 | LIVE | `ScriptInterp::op31TurnTowardTarget` | game/scene/script_interp.cpp:976 | 0x80085690 | op31 — FUN_80041468 (opcode table index 31). See script_interp.h for t… |
| 0x8004190C | LIVE | `Engine::animTick` | game/core/engine.cpp:1198 |  | Engine::animTick — FUN_8004190C. Ticks the animation VM (native |
| 0x8004201C | LIVE | `ScriptInterp::op04SceneFlagRendezvous` | game/scene/script_interp.cpp:258 |  | the SCENE-FLAG RENDEZVOUS opcode (table index 4). 1:1 with authenticat… |
| 0x80042090 | LIVE | `ScriptInterp::op05WaitFrames` | game/scene/script_interp.cpp:208 |  | VERIFIED + WIRED (frontier tier, 2026-07-10; return-value fix 2026-07-… |
| 0x800420AC | LIVE | `ScriptInterp::op06TestSceneFlag` | game/scene/script_interp.cpp:217 |  | VERIFIED + WIRED (frontier tier, 2026-07-10). 1:1 with authenticated e… |
| 0x80042170 | LIVE | `ScriptInterp::matchesActiveByKind` | game/scene/script_interp.cpp:1168 |  | ORACLE: guest 0x80042170 |
| 0x80042258 | LIVE | `SceneEvents::delayedTrigger` | game/scene/scene_events.cpp:133 |  | ORACLE: guest 0x80042258 |
| 0x80042258 | LIVE | `SceneEvents::delayedTriggerOverride` | game/scene/scene_events.cpp:199 |  |  |
| 0x80042310 | LIVE | `ActorTomba::resetLoadGate` | game/player/actor_tomba.cpp:1145 |  | resetLoadGate — guest FUN_80042310. See actor_tomba.h for the full RE … |
| 0x80042448 | LIVE | `SceneEvents::applyFlagOp` | game/scene/scene_events.cpp:173 |  | ORACLE: guest 0x80042448 |
| 0x80042448 | LIVE | `SceneEvents::applyFlagOpOverride` | game/scene/scene_events.cpp:202 |  |  |
| 0x80042728 | LIVE | `BgSceneTransitionSm::readyForProgress` | game/scene/bg_scene_transition_sm.cpp:277 |  |  |
| 0x80042758 | LIVE | `BgSceneTransitionSm::opSceneEventArmWait` | game/scene/bg_scene_transition_sm.cpp:290 | 0x80040B48 0x80042728 | - Cutscene-script opcode leaves (adjacent to readyForProgress in the g… |
| 0x80042884 | LIVE | `BgSceneTransitionSm::opClearSceneFlag80a` | game/scene/bg_scene_transition_sm.cpp:359 |  | opClearSceneFlag80a (FUN_80042884) — one-shot opcode leaf: clear the s… |
| 0x80042E10 | LIVE | `ScriptInterp::op34ClaimGate` | game/scene/script_interp.cpp:395 |  | VERIFIED + WIRED (frontier tier, 2026-07-10; §9 re-verify caught+fixed… |
| 0x80042EA4 | LIVE | `ScriptInterp::stepEventPulse` | game/scene/script_interp.cpp:731 |  | see script_interp.h for the full semantics writeup. |
| 0x80042EA4 | LIVE | `ScriptInterp::stepEventPulseFramed` | game/scene/script_interp.cpp:765 |  | Guest-ABI twin — mirrors FUN_80042EA4's own sp-=24 / ra-spill-at-+16 f… |
| 0x80043108 | LIVE | `ScriptInterp::op36MoveTowardScriptTarget` | game/scene/script_interp.cpp:783 | 0x80084080 0x80085690 | op36 — FUN_80043108 (opcode table index 36). See script_interp.h for t… |
| 0x80044090 | LIVE | `ScriptInterp::mirrorGlobalStatusByte` | game/scene/script_interp.cpp:1184 |  | ORACLE: guest 0x80044090 |
| 0x80044BD4 | LIVE | `native_area_load_bd4` | game/core/engine.cpp:2102 |  | Native replacement for FUN_80044bd4(0x800452c0, area, mode, 1): seed t… |
| 0x80044BD4 | LIVE | `Demo::s0PreYield` | game/scene/demo.cpp:664 |  |  |
| 0x80044BD4 | LIVE | `Sop::transitionAreaEnter` | game/scene/sop.cpp:170 |  | Synchronous TRANSITION area-DATA load — replaces the cooperative |
| 0x80044D8C | LIVE | `Asset::lzDecompress` | game/core/asset.cpp:31 |  |  |
| 0x80044E84 | LIVE | `Asset::unpackGroup` | game/core/asset.cpp:76 | 0x80080F6C | PC-owned texture-group unpacker — replaces guest FUN_80044E84 (0x80044… |
| 0x80044E84 | LIVE | `Asset::unpackGroupFaithful` | game/core/asset.cpp:145 | 0x80080F6C 0x80081218 | FAITHFUL texture-group unpacker — FUN_80044E84 with full guest-stack d… |
| 0x80044F58 | LIVE | `Asset::loadTexgroup` | game/core/asset.cpp:230 | 0x8001DC40 | PC-native TEXTURE-GROUP LOADER — owns the asset-load ORCHESTRATION FUN… |
| 0x80044F58 | LIVE | `Asset::preloadTexgroup` | game/core/asset.cpp:324 |  | texture-group load, synchronous. (Mirrors loadTexgroup but driven by e… |
| 0x800450BC | LIVE | `native_load_overlay` | game/core/engine.cpp:3796 |  | load the stage overlay (if any) and point the task's restart |
| 0x800450BC | LIVE | `eng_load_stage` | game/scene/level_load.cpp:25 | 0x8001DB8C 0x80080930 | load a stage's overlay off the disc and set the task's stage entry poi… |
| 0x8004514C | LIVE | `Asset::preloadStage1` | game/core/asset.cpp:414 |  | the stage-1 callback body. SWDATA + DAT load, shared texgroup sub-load… |
| 0x8004514C | LIVE | `Asset::preloadStage1AsTask` | game/core/asset.cpp:441 | 0x8001DC40 0x800754F4 | Task-1 body — FAITHFUL FUN_8004514C, run on a PcScheduler native fiber… |
| 0x80045258 | LIVE | `Asset::loadDescriptorChunk` | game/core/asset.cpp:615 |  | loadDescriptorChunk(descIdx, slot): FAITHFUL FUN_80045258 — a leaf ind… |
| 0x800452C0 | LIVE | `Asset::areaDataLoadAsTask` | game/core/asset.cpp:494 | 0x8001CF2C 0x8001DC40 0x80045080 0x80045558 0x80051F80 0x80051FB4 … | Task-1 body — FAITHFUL FUN_800452C0 (the walkable-field AREA-DATA load… |
| 0x800452C0 | LIVE | `native_area_load_bd4` | game/core/engine.cpp:2102 |  | Native replacement for FUN_80044bd4(0x800452c0, area, mode, 1): seed t… |
| 0x80045580 | LIVE | `ActorTomba::ov_turnBiasCompute` | game/player/actor_tomba.cpp:1060 |  | ov_turnBiasCompute/ov_outerTransitionGate/ov_outerTransitionCommit/ov_… |
| 0x80045580 | LIVE | `ActorTomba::assetReady` | game/player/actor_tomba.cpp:1158 |  | assetReady — guest FUN_80045580. See actor_tomba.h for the full RE wri… |
| 0x8004766C | LIVE | `Collision::snapObjectToTerrain` | game/player/collision.cpp:816 | 0x80047778 0x80047CBC 0x80048034 0x80048134 0x80049968 | Collision::snapObjectToTerrain. THE object-level entry point of the gr… |
| 0x8004798C | LIVE | `Collision::gridStep` | game/player/collision.cpp:719 | 0x8004798C |  |
| 0x80047CBC | LIVE | `Collision::gridQuery` | game/player/collision.cpp:493 | 0x80047CBC |  |
| 0x800498C8 | LIVE | `Collision::gridResolve` | game/player/collision.cpp:572 | 0x800498C8 |  |
| 0x80049968 | LIVE | `Collision::gridSetup` | game/player/collision.cpp:301 | 0x80049968 | collision-grid ROW-POINTER setup. a0 = grid/layer index (&0xff). Reads… |
| 0x800499E8 | LIVE | `Engine::task0Bootstrap` | game/core/engine.cpp:3838 |  | resolve \BIN\START.BIN natively, record its {LBA,size}, switch |
| 0x800499E8 | ORPHAN | `eng_task0_boot` | game/scene/level_load.cpp:101 | 0x8008A110 0x8008B8F0 0x8009A730 | task-0 INITIAL ENTRY (the engine's first-level bootstrap, registered a… |
| 0x80049A60 | LIVE | `ActorReward::smWindowScroll` | game/object/actor_sm_reward.cpp:174 |  | ActorReward::smWindowScroll(c) — FUN_80049A60(obj a0, side a1). Scroll… |
| 0x80049E54 | LIVE | `ActorReward::smTallyTick` | game/object/actor_sm_reward.cpp:333 |  | ActorReward::smTallyTick(c) — FUN_80049E54(obj a0, step a1) -> v0. Tic… |
| 0x8004A3D4 | LIVE | `ActorReward::smEventDispatch` | game/object/actor_sm_reward.cpp:389 |  | ActorReward::smEventDispatch(c) — FUN_8004A3D4(obj a0) -> v0. Mechanic… |
| 0x8004B150 | LIVE | `ActorReward::smBlinkA` | game/object/actor_sm_reward.cpp:123 |  | ActorReward::smBlinkA(c) — FUN_8004B150(obj a0, side a1). One-shot ini… |
| 0x8004B208 | LIVE | `ActorReward::smBlinkB` | game/object/actor_sm_reward.cpp:144 |  | ActorReward::smBlinkB(c) — FUN_8004B208(obj a0, side a1). Same shape a… |
| 0x8004B3F4 | LIVE | `Spawn::dropScoreGem` | game/world/spawn.cpp:784 | 0x80071B44 | SCORE-GEM DROP wrapper. Every callsite passes one of the eight fixed A… |
| 0x8004BD64 | LIVE | `GraphicsBind::posComposeBody` | game/world/graphics_bind.cpp:207 |  | per-object POSITION-COMPOSE + render-state refresh. RE'd from disas 0x… |
| 0x8004BD64 | LIVE | `GraphicsBind::posCompose` | game/world/graphics_bind.cpp:235 |  |  |
| 0x8004C238 | LIVE | `beh_visibility_gate_dispatch` | game/ai/beh_visibility_gate_dispatch.cpp:81 | 0x80049A60 0x80049E54 0x8004A118 0x8004A2A0 0x8004A3D4 0x8004B150 … |  |
| 0x8004C324 | LIVE | `state1_gate` | game/ai/beh_visibility_gate_dispatch.cpp:54 |  | --- STATE 1 shared VISIBILITY GATE (the body at 0x8004c324 / c3a4 / c4… |
| 0x8004CE14 | LIVE | `beh_record_list_scanner` | game/ai/beh_record_list_scanner.cpp:61 | 0x80111CCC |  |
| 0x8004D338 | LIVE | `Inventory::addNative` | game/items/inventory.cpp:74 |  | PC-native reimplementation of FUN_8004D338 (inventory_add). Writes are… |
| 0x8004D338 | LIVE | `Inventory::addBody` | game/items/inventory.cpp:113 |  | --- the FUN_8004D338 override + invverify gate -----------------------… |
| 0x8004D338 | LIVE | `Inventory::addEntry` | game/items/inventory.cpp:185 |  |  |
| 0x8004D338 | LIVE | `Inventory::add` | game/items/inventory.cpp:225 |  | --- PC-shape mutators: set the guest ABI regs and route through the st… |
| 0x8004D4C4 | LIVE | `Inventory::giveAndFlagBody` | game/items/inventory.cpp:195 | 0x8004ED0C | give_and_flag(type, amount): native add, then dispatch the PSX flag/ev… |
| 0x8004D4C4 | LIVE | `Inventory::giveAndFlagEntry` | game/items/inventory.cpp:202 |  |  |
| 0x8004D4C4 | LIVE | `Inventory::giveAndFlag` | game/items/inventory.cpp:235 |  |  |
| 0x8004D4F4 | LIVE | `Inventory::giveBody` | game/items/inventory.cpp:211 |  | give_only(type, amount): native add only. |
| 0x8004D4F4 | LIVE | `Inventory::giveEntry` | game/items/inventory.cpp:214 |  |  |
| 0x8004D4F4 | LIVE | `Inventory::give` | game/items/inventory.cpp:230 |  |  |
| 0x8004D7EC | LIVE | `Bit::test7EC` | game/math/mathlib.cpp:26 | 0x8004D7EC | pure bitmap bit-test (~2%, 6.8k calls): byte = bitmap[(int16)(idx/8)] … |
| 0x8004D868 | LIVE | `Bit::test868` | game/math/mathlib.cpp:56 | 0x8004D868 | sibling of FUN_8004D7EC (bit-test) against a fixed third bitmap @0x800… |
| 0x8004EB94 | LIVE | `emitSegmentLayout` | game/render/hud_gauge_emitter.cpp:188 |  | (descAddr, sign_extend16(spanBase + spanBias + bias)) call shape, shar… |
| 0x8004EB94 | LIVE | `gaugeTextRowTap` | game/render/hud_gauge_emitter.cpp:330 | 0x8004EB94 |  |
| 0x8004ED0C | LIVE | `Inventory::abGate` | game/items/inventory.cpp:126 |  | Full RAM+scratchpad A/B vs original guest-body call. The pure-leaf cor… |
| 0x8004ED94 | LIVE | `Engine::announcerCue` | game/core/engine.cpp:1219 | 0x8004FA38 | Engine::announcerCue — FUN_8004ED94. `id` sign-extended s16, then time… |
| 0x8004FA38 | LIVE | `Inventory::abGate` | game/items/inventory.cpp:126 |  | Full RAM+scratchpad A/B vs original guest-body call. The pure-leaf cor… |
| 0x8004FB20 | LIVE | `Pool::clearBf548Region` | game/world/pool.cpp:69 |  | zero 700 bytes at 0x800BF548. Trivial memset wrapper. Every field of t… |
| 0x8004FB4C | LIVE | `HudGaugeEmitter::emitItem` | game/render/hud_gauge_emitter.cpp:241 |  |  |
| 0x8004FD30 | LIVE | `HudGaugeEmitter::emitFrame` | game/render/hud_gauge_emitter.cpp:195 |  |  |
| 0x8004FE84 | LIVE | `Engine::sceneRenderListBuilder` | game/core/engine.cpp:866 |  | Native FUN_8004FE84 — a 2-phase scene/render-list builder driver (stru… |
| 0x8004FE84 | LIVE | `Engine::sceneRenderListBuilderFaithful` | game/core/engine.cpp:924 | 0x8004F430 0x8004F474 0x8004F514 0x8004F6D0 | Faithful mirror of guest 0x8004FE84 (authenticated executable/overlay … |
| 0x8004FFB4 | LIVE | `Panel::fillQuad` | game/ui/panel_fill.cpp:81 |  | EQUIVALENCE. This is a REBUILD, not a transcription, so `port_check` c… |
| 0x8004FFB4 | LIVE | `panelFillTap` | game/ui/panel.cpp:298 |  | installed via tomba::native::declareOverride() at game/ui/panel.cpp:35… |
| 0x8005019C | LIVE | `panelBuildTap` | game/ui/panel.cpp:318 | 0x8005019C |  |
| 0x8005082C | LIVE | `ModeStateArm::arm` | game/scene/mode_state_arm.cpp:10 |  | ModeStateArm::arm — native ownership of FUN_8005082C (Ghidra decomp sc… |
| 0x800508A8 | LIVE | `ModeStateArm::armFromAreaTable` | game/scene/mode_state_arm.cpp:30 |  | ModeStateArm::armFromAreaTable — native ownership of FUN_800508A8 (Ghi… |
| 0x80050970 | LIVE | `BgSceneTransitionSm::bf816Dispatch` | game/scene/bg_scene_transition_sm.cpp:115 |  | tiny dispatcher on the 800BF816 mode byte: 0 = ModeStateArm::armFromAr… |
| 0x800509B4 | LIVE | `Engine::initDisplay` | game/scene/startup.cpp:86 | 0x80050738 |  |
| 0x80050A0C | LIVE | `Engine::initFrameState` | game/scene/startup.cpp:58 |  |  |
| 0x80050A80 | LIVE | `Engine::initCamera` | game/scene/startup.cpp:123 |  | engine CAMERA init: identity camera-rotation matrix at scratchpad 0x1F… |
| 0x80050DE4 | LIVE | `Engine::sceneStateStepFaithful` | game/core/engine.cpp:3265 |  | Engine::sceneStateStep — the SCENE-INIT / SCENE-RUN state machine at g… |
| 0x80050DE4 | LIVE | `Engine::sceneStateStep` | game/core/engine.cpp:3348 |  | Engine::sceneStateStep — the SCENE-INIT / SCENE-RUN state machine at g… |
| 0x80051128 | LIVE | `NodeXform::propagate` | game/render/node_xform.cpp:337 |  | per-object CHILD-NODE TRANSFORM loop. RE'd from disas: |
| 0x80051300 | LIVE | `NodeXform::propagateRotmat` | game/render/node_xform.cpp:392 |  | per-object CHILD-NODE TRANSFORM loop, rotmat-single-call variant. RE'd… |
| 0x80051464 | LIVE | `NodeXform::propagateAxis` | game/render/node_xform.cpp:427 |  | sibling of propagateRotmat(): identical control flow, but the child's … |
| 0x80051614 | LIVE | `NodeXform::buildFromChild` | game/render/node_xform.cpp:529 |  | RE'd from authenticated executable/overlay evidence guest 0x80051614 (… |
| 0x80051794 | LIVE | `Mtx::identity` | game/math/mtx.cpp:6 |  |  |
| 0x80051794 | LIVE | `Mtx::registerOverrides` | game/math/mtx.cpp:47 |  |  |
| 0x800517BC | LIVE | `NodeXform::seedBlock` | game/render/node_xform.cpp:370 |  | trivial 8-word block seeder: {x,0,y,0,z,0,0,0}. RE'd + cross-checked v… |
| 0x800517F8 | LIVE | `GraphicsBind::renderUpdateBody` | game/world/graphics_bind.cpp:132 | 0x80051300 | per-object RENDER-STATE UPDATE: build the object's transform, then sna… |
| 0x800517F8 | LIVE | `GraphicsBind::renderUpdate` | game/world/graphics_bind.cpp:155 |  |  |
| 0x80051844 | LIVE | `NodeXform::build` | game/render/node_xform.cpp:265 |  | REGISTER FAITHFULNESS (2026-07-08, the f117 residual root cause): fram… |
| 0x800518FC | LIVE | `NodeXform::buildWithOffset` | game/render/node_xform.cpp:301 |  | NodeXform::buildWithOffset — PC-native reimpl of guest FUN_800518FC. |
| 0x800519E0 | LIVE | `GraphicsBind::recordArrayInit` | game/world/graphics_bind.cpp:299 |  |  |
| 0x80051B04 | LIVE | `GraphicsBind::installSceneRecord` | game/world/graphics_bind.cpp:109 |  | two-level scene-data-table pointer resolve. Pure address arithmetic, n… |
| 0x80051B34 | LIVE | `NodeXform::copyMatrixBlock` | game/render/node_xform.cpp:494 |  | frameless leaf, verbatim from authenticated executable/overlay evidenc… |
| 0x80051B70 | LIVE | `GraphicsBind::recordInitBody` | game/world/graphics_bind.cpp:50 |  | per-object render-record INIT. Allocates a record (FUN_8007AAE8), zero… |
| 0x80051B70 | LIVE | `GraphicsBind::recordInit` | game/world/graphics_bind.cpp:98 |  |  |
| 0x80051C8C | LIVE | `NodeXform::buildAxis` | game/render/node_xform.cpp:465 |  | node-level sibling of build(): composes THIS node's own world matrix v… |
| 0x80051D20 | LIVE | `NodeXform::worldPosFromComposed` | game/render/node_xform.cpp:601 |  | sibling of worldPosFromLocal() using node's COMPOSED world matrix and … |
| 0x80051D90 | LIVE | `NodeXform::worldPosFromLocal` | game/render/node_xform.cpp:584 |  | RE'd from authenticated executable/overlay evidence guest 0x80051D90 (… |
| 0x80052078 | LIVE | `Engine::startStage` | game/core/engine.cpp:3818 | 0x80080870 0x80080890 0x800808A0 | switch task 0 to the given stage (load overlay + reset the |
| 0x80052078 | LIVE | `eng_stage_transition` | game/scene/level_load.cpp:70 |  | (stageIdx) — the cooperative STAGE TRANSITION: load the next stage's o… |
| 0x800520E0 | LIVE | `Engine::initSubsystems` | game/scene/startup.cpp:314 |  |  |
| 0x8005229C | LIVE | `Engine::padFenceTail` | game/input/pad_edge_fence.cpp:145 | 0x80087AEC 0x80087E2C 0x80087EAC | Override wrapper + install (guest ABI is all-implicit — the fence take… |
| 0x8005229C | LIVE | `ov_padFenceTail` | game/input/pad_edge_fence.cpp:351 |  |  |
| 0x800527C8 | LIVE | `beh_actor_tomba_proximity_combat` | game/ai/beh_actor_tomba_proximity_combat.cpp:48 | 0x80041718 0x80041768 0x8004190C 0x80042728 0x800518FC 0x800519E0 … |  |
| 0x80053E50 | LIVE | `ActorTomba::outerTransitionGate` | game/player/actor_tomba.cpp:1181 |  |  |
| 0x80053FDC | LIVE | `ActorTomba::outerTransitionCommit` | game/player/actor_tomba.cpp:1246 |  | outerTransitionCommit — guest FUN_80053FDC(G, mode). See actor_tomba.h… |
| 0x80054198 | LIVE | `SceneTransition::clearSwapBlock` | game/scene/scene_transition.cpp:130 |  | small swap-block ephemeral clear. RE'd from disas 0x80054198..0x800541… |
| 0x80054650 | LIVE | `ActorTomba::settleStep` | game/player/actor_tomba.cpp:929 | 0x8004954C | ======================================================================… |
| 0x80054D14 | LIVE | `Engine::walkStart` | game/core/engine.cpp:1242 |  | Engine::walkStart — FUN_80054D14. |
| 0x80055C9C | LIVE | `gov_turnBiasCompute` | game/player/actor_tomba.cpp:1073 |  | installed via tomba::native::declareOverride() at game/player/actor_to… |
| 0x80056B48 | LIVE | `ActorTomba::velocityIntegrate` | game/player/actor_tomba.cpp:992 |  | ======================================================================… |
| 0x80057DC0 | LIVE | `ActorTomba::growthStep` | game/player/actor_tomba.cpp:552 |  | ======================================================================… |
| 0x80058304 | LIVE | `Engine::gStateMutate` | game/core/engine.cpp:1355 | 0x800310F4 | Engine::gStateMutate — native ownership of FUN_80058304 (Ghidra decomp |
| 0x8005950C | LIVE | `ActorTomba::frameTick` | game/player/actor_tomba.cpp:1328 |  |  |
| 0x80059D28 | LIVE | `Engine::frameStartTick` | game/core/engine.cpp:3572 |  | Engine::frameStartTick — per-frame prologue at guest 0x80059D28 (FIRST… |
| 0x80059D28 | LIVE | `Engine::frameStartTickFaithful` | game/core/engine.cpp:3663 | 0x8005950C 0x8009A450 0x80109024 0x8010F63C 0x8010F654 0x80112220 | Engine::frameStartTickFaithful — byte-exact mirror of guest 0x80059D28 |
| 0x80059ED8 | LIVE | `beh_camera_target_follow` | game/ai/beh_camera_target_follow.cpp:54 | 0x800312D4 0x800489E4 0x8010B238 0x8010BC10 0x8010C5A8 0x8011332C … |  |
| 0x8005A910 | LIVE | `ActorTomba::mode0ActionGate` | game/player/actor_tomba.cpp:1017 |  |  |
| 0x80067DA8 | LIVE | `Engine::uploadModeSprites` | game/core/engine.cpp:1289 | 0x80081218 | Engine::uploadModeSprites — native ownership of FUN_80067DA8 (Ghidra d… |
| 0x80069B28 | LIVE | `ObjectList::walkAuxFaithful` | game/object/object_list.cpp:156 |  | pc_faithful mirror of guest 0x80069B28 (guest FUN_80069B28). Guest fra… |
| 0x8006C80C | LIVE | `CutsceneCamera::yFloor` | game/camera/cutscene_camera.cpp:431 |  | ── yFloor (camera-Y floor clamp, per render mode) ────────────────────… |
| 0x8006C988 | LIVE | `CutsceneCamera::shakeTail` | game/camera/cutscene_camera.cpp:895 |  | ── post-mode TAIL (0x8006C988) — the camera SHAKE state machine ──────… |
| 0x8006CBA8 | LIVE | `CutsceneCamera::initSeedGrp` | game/camera/cutscene_camera.cpp:1105 |  |  |
| 0x8006CBD0 | LIVE | `GraphicsBind::setXformBlkBody` | game/world/graphics_bind.cpp:181 |  | copy a 6-halfword TRANSFORM BLOCK from a1 into the scratchpad camera/t… |
| 0x8006CBD0 | LIVE | `GraphicsBind::setXformBlk` | game/world/graphics_bind.cpp:192 |  |  |
| 0x8006D02C | LIVE | `CutsceneCamera::lookAt` | game/camera/cutscene_camera.cpp:714 |  |  |
| 0x8006D2AC | LIVE | `CutsceneCamera::distSolve` | game/camera/cutscene_camera.cpp:259 |  | ── distSolve (distance/zoom solver) ──────────────────────────────────… |
| 0x8006D654 | LIVE | `CutsceneCamera::pitch` | game/camera/cutscene_camera.cpp:490 |  | ── pitch (vertical-look height smoother) ─────────────────────────────… |
| 0x8006D934 | LIVE | `CutsceneCamera::snapAccXZ` | game/camera/cutscene_camera.cpp:795 |  | ── orchestrators (per-frame camera modes) ────────────────────────────… |
| 0x8006D950 | LIVE | `CutsceneCamera::snapAccY` | game/camera/cutscene_camera.cpp:799 |  |  |
| 0x8006D960 | LIVE | `CutsceneCamera::trackXZ` | game/camera/cutscene_camera.cpp:74 |  | ── follow accumulators ───────────────────────────────────────────────… |
| 0x8006DA54 | LIVE | `CutsceneCamera::trackY` | game/camera/cutscene_camera.cpp:82 |  |  |
| 0x8006DAD8 | LIVE | `CutsceneCamera::posBuildB` | game/camera/cutscene_camera.cpp:120 |  |  |
| 0x8006DC38 | LIVE | `CutsceneCamera::posBuildA` | game/camera/cutscene_camera.cpp:111 |  | ── scripted-camera look-angle builders (0x8006DC38/DAD8/DF88/DEF0 — us… |
| 0x8006DCF4 | LIVE | `CutsceneCamera::heading` | game/camera/cutscene_camera.cpp:620 |  | ── heading (heading tracker) ─────────────────────────────────────────… |
| 0x8006DEF0 | LIVE | `CutsceneCamera::headBuildB` | game/camera/cutscene_camera.cpp:138 |  |  |
| 0x8006DF88 | LIVE | `CutsceneCamera::headBuildA` | game/camera/cutscene_camera.cpp:128 |  |  |
| 0x8006E010 | LIVE | `CutsceneCamera::angleStep` | game/camera/cutscene_camera.cpp:380 |  | ── angleStep ─────────────────────────────────────────────────────────… |
| 0x8006E0F0 | LIVE | `CutsceneCamera::mainFollow` | game/camera/cutscene_camera.cpp:850 |  |  |
| 0x8006E1C0 | LIVE | `CutsceneCamera::pushMode` | game/camera/cutscene_camera.cpp:1131 |  |  |
| 0x8006E1E4 | LIVE | `CutsceneCamera::restoreMode` | game/camera/cutscene_camera.cpp:1138 |  |  |
| 0x8006E228 | LIVE | `CutsceneCamera::trackFollow` | game/camera/cutscene_camera.cpp:875 |  |  |
| 0x8006E294 | LIVE | `CutsceneCamera::snapFollowA` | game/camera/cutscene_camera.cpp:826 |  |  |
| 0x8006E2FC | LIVE | `CutsceneCamera::snapFollowB` | game/camera/cutscene_camera.cpp:841 |  |  |
| 0x8006E360 | LIVE | `CutsceneCamera::pitchFollow` | game/camera/cutscene_camera.cpp:835 |  |  |
| 0x8006E3B0 | LIVE | `CutsceneCamera::snapFollow` | game/camera/cutscene_camera.cpp:802 |  |  |
| 0x8006E3F4 | LIVE | `CutsceneCamera::simpleFollow` | game/camera/cutscene_camera.cpp:866 |  |  |
| 0x8006E464 | LIVE | `CutsceneCamera::rotBuild` | game/camera/cutscene_camera.cpp:226 |  |  |
| 0x8006E8F8 | LIVE | `CutsceneCamera::resetFollowAccum` | game/camera/cutscene_camera.cpp:1125 |  | ── Wiring pass (2026-07-08 frontier follow-up) ───────────────────────… |
| 0x8006E918 | LIVE | `CutsceneCamera::initPlace` | game/camera/cutscene_camera.cpp:1073 |  |  |
| 0x8006EA00 | LIVE | `CutsceneCamera::snapToMasterOffsetY200` | game/camera/cutscene_camera.cpp:1159 |  | pushes a real 32-byte guest frame (r29-=32, s0/s1/ra spilled at +16/+2… |
| 0x8006EA7C | ORPHAN | `CutsceneCamera::init` | game/camera/cutscene_camera.cpp:1448 |  |  |
| 0x8006EC44 | LIVE | `CutsceneCamera::update` | game/camera/cutscene_camera.cpp:1216 |  |  |
| 0x8006EC44 | LIVE | `CutsceneCamera::updateFaithful` | game/camera/cutscene_camera.cpp:1258 | 0x8006C988 0x8006EA7C | pc_faithful mirror of guest 0x8006EC44 (authenticated executable/overl… |
| 0x8006EF38 | LIVE | `CutsceneCamera::orbitTick` | game/camera/cutscene_camera.cpp:1190 |  | pushes the same shape of 32-byte frame (r29-=32, s0/s1/ra spilled at +… |
| 0x8006EFF4 | LIVE | `Bit::testFE48` | game/math/mathlib.cpp:84 |  | u32 flag-bit TEST on the fixed 32-bit word at 0x800BFE48. Pure 5-instr… |
| 0x8006F00C | LIVE | `Bit::setFE48` | game/math/mathlib.cpp:98 |  | sibling of setFE34: u32 flag-bit SET on 0x800BFE48 (the word testFE48 … |
| 0x8006F02C | LIVE | `Bit::setFE34` | game/math/mathlib.cpp:91 |  | u32 flag-bit SET on the fixed 32-bit word at 0x800BFE34. 7-instruction… |
| 0x8006F04C | LIVE | `Bit::processLinkRequest` | game/math/mathlib.cpp:114 |  | child-link REQUEST-mailbox arbiter. disas 0x8006F04C..0x8006F0E0: |
| 0x8006F2D0 | LIVE | `beh_pad_child_linker` | game/ai/beh_pad_child_linker.cpp:63 | 0x8004766C 0x80047B5C 0x8006F138 |  |
| 0x80070018 | LIVE | `ActorReward::update` | game/object/actor_sm_reward.cpp:701 |  |  |
| 0x800702C0 | LIVE | `ActorReward::resolvePosition` | game/object/actor_sm_reward.cpp:915 |  |  |
| 0x80070650 | LIVE | `ActorReward::approachTargetX` | game/object/actor_sm_reward.cpp:1027 |  | ActorReward::approachTargetX(c) — FUN_80070650(obj a0). Trivial ease: … |
| 0x80071A3C | LIVE | `beh_area_event_dispatch` | game/ai/beh_area_event_dispatch.cpp:45 | 0x800716B4 0x80071768 0x801178E4 0x8011B79C |  |
| 0x80072520 | LIVE | `ScorePopup::install` | game/render/score_popup.cpp:98 |  |  |
| 0x800726D4 | LIVE | `Render::fadeTileRender` | game/render/screen_fade.cpp:253 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80072A78 | LIVE | `Placement::placeAreaObjects` | game/world/placement.cpp:144 | 0x80072A78 |  |
| 0x80072DDC | LIVE | `Placement::spawnWithParent` | game/world/placement.cpp:211 | 0x80072DDC |  |
| 0x80073194 | LIVE | `ScriptInterp::advanceGauge` | game/scene/script_interp.cpp:1198 | 0x80074590 | ORACLE: guest 0x80073194 |
| 0x80073260 | LIVE | `SceneTransition::resetSwap` | game/scene/scene_transition.cpp:113 |  |  |
| 0x800732C0 | LIVE | `SceneTransition::beginSwap` | game/scene/scene_transition.cpp:148 |  |  |
| 0x80073300 | LIVE | `SceneTransition::completeSwap` | game/scene/scene_transition.cpp:155 |  |  |
| 0x80073328 | LIVE | `SceneTransition::stepSwapWaiter` | game/scene/scene_transition.cpp:163 | 0x80073328 |  |
| 0x800735F4 | LIVE | `Spawn::tickLinkedOverlay` | game/world/spawn.cpp:875 |  | per-object controller that owns exactly ONE linked "variant overlay" c… |
| 0x80073750 | LIVE | `Font::measureLineWidth` | game/ui/font.cpp:202 |  | pure string measurer (disas 0x80073750..0x80073798, no sub-calls): |
| 0x800739AC | LIVE | `beh_scene_ui_trigger` | game/ai/beh_scene_ui_trigger.cpp:60 | 0x800737F8 0x800738B0 0x80074BF8 |  |
| 0x80073CD8 | LIVE | `beh_typed_init_scene_trigger` | game/ai/beh_typed_init_scene_trigger.cpp:107 |  |  |
| 0x800741DC | LIVE | `beh_pickup_collect_trigger` | game/ai/beh_pickup_collect_trigger.cpp:213 |  |  |
| 0x80074590 | LIVE | `Sfx::trigger` | game/audio/sfx.cpp:16 | 0x80074BF8 0x80074EEC 0x80075E04 |  |
| 0x80074810 | LIVE | `Sfx::triggerPanned` | game/audio/sfx.cpp:165 | 0x80074590 | ORACLE: guest 0x80074810 |
| 0x80074810 | LIVE | `Sfx::registerOverrides` | game/audio/sfx.cpp:196 |  |  |
| 0x8007496C | LIVE | `AreaSlots::updateCell` | game/world/area_slots.cpp:277 | 0x80092E3C | AreaSlots::updateCell — FUN_8007496C body. sigArg carries {idx: low by… |
| 0x80074A38 | LIVE | `AreaSlots::primeCountdown` | game/world/area_slots.cpp:265 |  | AreaSlots::primeCountdown — FUN_80074A38 body. Pure 1-store leaf: tabl… |
| 0x80074A38 | LIVE | `AreaSlots::registerOverrides` | game/world/area_slots.cpp:382 |  |  |
| 0x80074AF0 | LIVE | `AreaSlots::ackIfMatch` | game/world/area_slots.cpp:250 |  | AreaSlots::ackIfMatch — FUN_80074AF0 body. Pure 21-instruction primiti… |
| 0x80074BC4 | LIVE | `AudioDispatch::settleField` | game/audio/audio_dispatch.cpp:86 | 0x8001CF2C 0x80074B44 0x80074E48 | AudioDispatch::settleField — native ownership of FUN_80074BC4 (Ghidra … |
| 0x80074F24 | LIVE | `Pool::selectStateIndex` | game/world/pool.cpp:350 |  | per-area STATE-INDEX select + apply. Early-out if scratchpad 0x1F80013… |
| 0x80075024 | LIVE | `AudioDispatch::selectStateRemap` | game/audio/audio_dispatch.cpp:150 | 0x800750D8 | AudioDispatch::selectStateRemap — native ownership of FUN_80075024. Ma… |
| 0x80075070 | LIVE | `AudioDispatch::publishStateFade` | game/audio/audio_dispatch.cpp:188 | 0x80075CEC | AudioDispatch::publishStateFade — native ownership of FUN_80075070. Pu… |
| 0x800750A4 | LIVE | `AudioDispatch::selectState` | game/audio/audio_dispatch.cpp:102 |  | AudioDispatch::selectState — native ownership of FUN_800750A4 (Ghidra … |
| 0x800750D8 | LIVE | `AudioDispatch::dispatch3Way` | game/audio/audio_dispatch.cpp:32 | 0x8001CF2C | AudioDispatch::dispatch3Way — native ownership of FUN_800750D8 (Ghidra… |
| 0x80075130 | LIVE | `Font::init` | game/ui/font.cpp:129 |  | font / text system init orchestrator. No args, no return. Mirrors the … |
| 0x80075240 | LIVE | `Pool::reset75240` | game/world/pool.cpp:192 |  | reset the control block at 0x800BE1F8: call 0x80075D58 leaf, seed clam… |
| 0x800752B4 | LIVE | `Font::glyphClassFill` | game/ui/font.cpp:103 |  | glyph-class table fill. Iterates i = 0..23 over the 24-entry table. Th… |
| 0x800753AC | LIVE | `preload_build_vram` | game/core/asset.cpp:379 | 0x80075448 | cel/sprite VRAM build, synchronous. FUN_800753ac is itself an async CD… |
| 0x800753D4 | LIVE | `preload_cel` | game/core/asset.cpp:345 | 0x80096480 0x80096980 0x80096A40 | cel-load, SYNCHRONOUS. Original: FUN_80096480 (slot alloc + BAV cel lo… |
| 0x800753D4 | LIVE | `preload_build_vram` | game/core/asset.cpp:379 | 0x80075448 | cel/sprite VRAM build, synchronous. FUN_800753ac is itself an async CD… |
| 0x80075448 | LIVE | `preload_build_vram` | game/core/asset.cpp:379 | 0x80075448 | cel/sprite VRAM build, synchronous. FUN_800753ac is itself an async CD… |
| 0x800754F4 | LIVE | `preload_build_vram` | game/core/asset.cpp:379 | 0x80075448 | cel/sprite VRAM build, synchronous. FUN_800753ac is itself an async CD… |
| 0x80075824 | LIVE | `MusicCoord::voiceMixTick` | game/audio/music_coord.cpp:140 |  | Per-frame VOICE-CHANNEL VOLUME MIXER — port of FUN_80075824 (RE'd via … |
| 0x80075A80 | LIVE | `AreaSlots::updateTail` | game/world/area_slots.cpp:44 | 0x80074BF8 0x80074E48 0x8008E0C0 0x80092660 0x80098F90 0x80099490 … | AreaSlots::updateTail — the last direct child of ov_field_frame at gue… |
| 0x80075CEC | LIVE | `BgSceneTransitionSm::audioFadeTarget` | game/scene/bg_scene_transition_sm.cpp:75 |  | - Native ports of the tiny sub-leaves this SM calls ------------------… |
| 0x80075D24 | LIVE | `MusicCoord::setGain2` | game/audio/music_coord.cpp:247 |  | MusicCoord::setGain2 — FUN_80075D24 body. See music_coord.h for the RE… |
| 0x80075D24 | LIVE | `MusicCoord::registerOverrides` | game/audio/music_coord.cpp:277 |  |  |
| 0x80075F0C | LIVE | `Animation::applyFrame` | game/object/animation.cpp:627 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80076904 | LIVE | `Animation::loadFrame` | game/object/animation.cpp:393 |  |  |
| 0x80076904 | LIVE | `Animation::registerOverrides` | game/object/animation.cpp:692 |  |  |
| 0x80076D68 | LIVE | `Animation::stepFramed` | game/object/animation.cpp:247 |  | Animation::stepFramed — GUEST-ABI ENTRY ONLY for FUN_80076D68 (RE: aut… |
| 0x8007703C | LIVE | `Cull::enqueueByClass` | game/render/cull.cpp:383 |  | Cull::enqueueByClass — PC-native FUN_8007703C body. Class-keyed queue … |
| 0x8007712C | LIVE | `Cull::decide` | game/render/cull.cpp:120 |  | Pure (read-only) cull decision — reproduces FUN_8007712c's control flo… |
| 0x8007712C | LIVE | `Cull::performBaseCull` | game/render/cull.cpp:210 |  | Cull::performBaseCull — byte-exact PC-native FUN_8007712C body (no mar… |
| 0x8007712C | LIVE | `Cull::objectCull` | game/render/cull.cpp:442 |  |  |
| 0x8007712C | LIVE | `Cull::performBaseCullFramed` | game/render/cull.cpp:659 |  | performBaseCullFramed — mirrors FUN_8007712C's OWN real 40-byte guest-… |
| 0x80077768 | LIVE | `Trig::angleCmp` | game/math/trig.cpp:80 |  |  |
| 0x8007778C | LIVE | `Cull::wrapFrame` | game/render/cull.cpp:634 |  | camera-relative cull WRAPPER. Computes obj-cam delta (wrapping s16, si… |
| 0x8007778C | LIVE | `Cull::cullWrapper` | game/render/cull.cpp:682 |  |  |
| 0x800777FC | LIVE | `Cull::cullWrapperFlag2` | game/render/cull.cpp:750 |  | UNFRAMED — the public entry point EXISTING native beh_ callers (beh_id… |
| 0x80077870 | LIVE | `Cull::cullWrapperFlag1` | game/render/cull.cpp:714 |  | cull-wrapper variant: byte-identical to cullWrapper (obj in c->r[4], d… |
| 0x800778E4 | LIVE | `Cull::cullWrapperOffsetY` | game/render/cull.cpp:858 |  |  |
| 0x800779D0 | LIVE | `Cull::cullWrapperOffset` | game/render/cull.cpp:814 |  |  |
| 0x80077A4C | LIVE | `Cull::cullWrapperOffsetFlag1` | game/render/cull.cpp:835 |  |  |
| 0x80077ACC | LIVE | `Cull::cullWrap77acc` | game/render/cull.cpp:784 |  | UNFRAMED — the public entry point EXISTING native callers (beh_record_… |
| 0x80077B38 | LIVE | `GraphicsBind::setGeomBody` | game/world/graphics_bind.cpp:163 |  | set an object's GEOMETRY-BLOCK pointer from a table. RE'd from disas 0… |
| 0x80077B38 | LIVE | `GraphicsBind::setGeom` | game/world/graphics_bind.cpp:171 |  |  |
| 0x80077B5C | LIVE | `Animation::advanceLinkChain` | game/object/animation.cpp:509 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80077C40 | LIVE | `Animation::attach` | game/object/animation.cpp:566 | 0x80075FF8 | ──────────────────────────────────────────────────────────────────────… |
| 0x80077D8C | LIVE | `Engine::postRenderTickFaithful` | game/core/engine.cpp:3515 | 0x80074590 | Engine::postRenderTickFaithful -- byte-exact mirror of guest 0x80077D8… |
| 0x80077E7C | LIVE | `Cull::enqueueQueueA` | game/render/cull.cpp:405 |  | Cull::enqueueQueueA — PC-native FUN_80077E7C body. Manual push of `obj… |
| 0x80077EBC | LIVE | `Cull::enqueueVisibleClass4` | game/render/cull.cpp:361 |  | Cull::enqueueVisibleClass4 — PC-native FUN_80077EBC body. Manual push … |
| 0x80077EFC | LIVE | `Cull::enqueueQueueC` | game/render/cull.cpp:425 |  | Cull::enqueueQueueC — PC-native FUN_80077EFC body. Manual push onto qu… |
| 0x80077FB0 | LIVE | `eov_isqrt16` | game/math/gte_math.cpp:864 |  | installed via tomba::native::declareOverride() at game/math/gte_math.c… |
| 0x80078240 | LIVE | `Trig::vecLen` | game/math/trig.cpp:112 |  | vecLen (guest FUN_80078240) — the integer 3-D length approximation. Th… |
| 0x80078240 | LIVE | `eov_approxDist3` | game/math/gte_math.cpp:867 |  | installed via tomba::native::declareOverride() at game/math/gte_math.c… |
| 0x800782F0 | LIVE | `SceneTransition::areaMaskTrigger` | game/scene/scene_transition.cpp:28 | 0x800782F0 |  |
| 0x800783DC | LIVE | `Pool::setupViewScroll` | game/world/pool.cpp:215 |  | per-area VIEW/SCROLL setup. Calls a leaf (0x80048D3C), builds the view… |
| 0x80078610 | LIVE | `Pool::finalViewInit` | game/world/pool.cpp:295 |  | final per-area view init: zero two control blocks, seed fixed view par… |
| 0x80078824 | LIVE | `Engine::setAreaStartPos` | game/core/engine.cpp:4324 |  | Engine::setAreaStartPos. Loads the player's per-area spawn |
| 0x800788AC | LIVE | `Engine::padEdgeFence` | game/input/pad_edge_fence.cpp:51 |  | per-frame input-edge fence. See the file header above for the full RE … |
| 0x800788AC | LIVE | `ov_padEdgeFence` | game/input/pad_edge_fence.cpp:348 |  |  |
| 0x80078988 | LIVE | `Font::iconGlyphEmit` | game/ui/font.cpp:841 |  | iconGlyphEmit — FUN_80078988, the SJIS/token ICON-GLYPH string emitter… |
| 0x80078CA8 | LIVE | `Font::glyphQueuePush` | game/ui/font.cpp:308 |  | the font/glyph emitter drawText() tail-calls. WIDE-RE TIER DRAFT (2026… |
| 0x80078CA8 | LIVE | `Font::glyphEmit` | game/ui/font.cpp:354 | 0x80078988 0x80083DE0 |  |
| 0x80079324 | LIVE | `Font::drawTextSmall` | game/ui/font.cpp:271 |  | ORACLE: guest 0x80079324 |
| 0x80079324 | LIVE | `ov_drawTextSmall` | game/ui/font.cpp:706 |  | ov_drawTextSmall: sibling of ov_drawText for FUN_80079324 — same guest… |
| 0x80079374 | LIVE | `Font::drawText` | game/ui/font.cpp:245 |  | WIDE-RE TIER DRAFT (2026-07-09), UNWIRED/UNVERIFIED. See header doc fo… |
| 0x80079374 | LIVE | `ov_drawText` | game/ui/font.cpp:695 |  | ov_drawText: extracts drawText's typed args from the guest ABI registe… |
| 0x80079528 | LIVE | `Str::length` | game/core/str.cpp:16 |  | strlen. RE (tools/disas.py 0x80079528 --all 20, cross-checked against |
| 0x80079528 | LIVE | `ov_strLength` | game/core/str.cpp:63 |  |  |
| 0x800796DC | LIVE | `Pool::resetControlBlock` | game/world/pool.cpp:23 |  | zero the 104-byte control block at 0x800BF808, seed two bytes, clear ~… |
| 0x800798F8 | LIVE | `Pool::initTypedPools` | game/world/pool.cpp:81 |  | the 5 typed object pools + list-head init. See pool.h for the pool tab… |
| 0x80079C3C | LIVE | `Spawn::spawnLinkStamp` | game/world/spawn.cpp:69 |  | Link `node` into active list `list` at position `mode` relative to `re… |
| 0x80079C3C | LIVE | `Spawn::entitySpawnBody` | game/world/spawn.cpp:141 |  |  |
| 0x80079DDC | LIVE | `Spawn::spawnPool2Body` | game/world/spawn.cpp:165 |  |  |
| 0x80079F90 | LIVE | `Spawn::poolSpawn` | game/world/spawn.cpp:224 |  |  |
| 0x8007A624 | LIVE | `Spawn::despawn` | game/world/spawn.cpp:303 | 0x8007A624 |  |
| 0x8007A904 | LIVE | `ObjectList::walkAllFaithful` | game/object/object_list.cpp:89 |  | pc_faithful mirror of guest 0x8007A904 (guest FUN_8007A904). Guest fra… |
| 0x8007A980 | LIVE | `Spawn::dispatch` | game/world/spawn.cpp:197 |  | Run the per-class spawn VARIANT NATIVELY (the 5 bodies are all owned i… |
| 0x8007AAE8 | LIVE | `GraphicsBind::recordAllocBody` | game/world/graphics_bind.cpp:28 |  | ======================================================================… |
| 0x8007AAE8 | LIVE | `GraphicsBind::recordAlloc` | game/world/graphics_bind.cpp:78 |  |  |
| 0x8007B008 | LIVE | `ObjectList::walkList2` | game/object/object_list.cpp:119 |  |  |
| 0x8007B04C | LIVE | `TransitionState3::walkOnce` | game/scene/transition_state3.cpp:11 |  |  |
| 0x8007B18C | LIVE | `Pool::init` | game/world/pool.cpp:138 |  | top-level object-pool init. Zeroes 520 68-byte slots at 0x800F2740; bu… |
| 0x8007B2C0 | LIVE | `Engine::seedDirectionMasks` | game/scene/startup.cpp:168 |  | direction-mask seeder. Called with 0 at boot (initEntityPool above) an… |
| 0x8007B328 | LIVE | `Engine::initEntityPool` | game/scene/startup.cpp:149 |  | engine SUBSYSTEM init (init-prefix slot, dispatched at native_boot.cpp… |
| 0x8007B3F4 | LIVE | `Engine::reloadEntityPool` | game/scene/startup.cpp:185 |  | re-copy the staged per-area entity-pool control bytes onto the live he… |
| 0x8007CC00 | LIVE | `Panel::pushDialogGlyphs` | game/ui/panel.cpp:241 |  | pushDialogGlyphs — Spec 3, FUN_8007CC00 (gen shard_4.c:11855): the dia… |
| 0x8007D0D0 | LIVE | `DialogTextStream::applyRenderMode` | game/ui/dialog_text_stream.cpp:44 |  | (obj a0) -- LEAF (guest 0x8007D0D0 has no `sp` descent). Cross-checked… |
| 0x8007DC38 | LIVE | `beh_variant_overlay_lifecycle` | game/ai/beh_variant_overlay_lifecycle.cpp:54 | 0x8007C0D0 | NOT port_check-able as it stands: this is a hand-written REBUILD, not … |
| 0x8007E038 | LIVE | `Spawn::spawnOverlayVariantBody` | game/world/spawn.cpp:810 |  | VARIANT-OVERLAY SPAWN primitive. RE'd from disas 0x8007E038..0x8007E10… |
| 0x8007E038 | LIVE | `Spawn::spawnOverlayVariant` | game/world/spawn.cpp:850 |  |  |
| 0x8007E110 | LIVE | `Spawn::sceneEntityBody` | game/world/spawn.cpp:725 |  | SCENE-ENTITY SPAWN primitive. RE'd from disas 0x8007E110..0x8007E1B4. |
| 0x8007E110 | LIVE | `Spawn::sceneEntity` | game/world/spawn.cpp:762 |  |  |
| 0x8007E1B8 | LIVE | `Render::emitMenuFt4` | game/render/render_walk.cpp:312 |  | emitMenuFt4 / emitMenuSprites — the MENU-specialized wrappers over the… |
| 0x8007E1B8 | LIVE | `uiFt4Tap` | game/render/ui_ft4_tap.cpp:16 |  | installed via tomba::native::declareOverride() at game/render/ui_ft4_t… |
| 0x8007E6DC | LIVE | `Render::emitUiSprites` | game/render/field_hud.cpp:315 |  | --- emitUiSprites — general FUN_8007E6DC (SPRT template group) -------… |
| 0x8007E6DC | LIVE | `Render::emitMenuSprites` | game/render/render_walk.cpp:317 |  |  |
| 0x8007E6DC | LIVE | `ov_compose` | game/ui/ui_sprite.cpp:101 |  | The pause/item menu, the START page and the score popup all paint thro… |
| 0x8007E6DC | LIVE | `UiSprite::compose` | game/ui/ui_sprite_compose.cpp:51 | 0x80083DE0 | (placement r4, indexPtr r5, defBase r6, attrs r7) |
| 0x8007E8DC | LIVE | `UiSprite::drawFromTable` | game/ui/ui_sprite.cpp:43 | 0x8007E1B8 | (x r4, y r5, attr r6, defIndex r7) |
| 0x8007E998 | LIVE | `UiSprite::drawFixedDef152` | game/ui/ui_sprite.cpp:76 | 0x8007E8DC | (x r4, y r5, attr r6) — drawFromTable with the definition index pinned… |
| 0x8007E9C8 | LIVE | `ScreenFade::fadetrace` | game/render/screen_fade.cpp:20 |  | `debug fadetrace` channel — logs every native-path fade call with the … |
| 0x8007E9C8 | LIVE | `ScreenFade::installLeafTap` | game/render/screen_fade.cpp:98 |  |  |
| 0x8007E9C8 | LIVE | `BgSceneTransitionSm::fadeRect` | game/scene/bg_scene_transition_sm.cpp:62 |  | Screen fade — same shape as the guest's FUN_8007e9c8(color, P[3], 4) l… |
| 0x8007E9C8 | LIVE | `PauseMenu::releaseGlobalDim` | game/ui/pause_menu.cpp:81 |  | DOUBLE OWNERSHIP OF THE MENU DIM (kanban #59 — the "menu chrome too da… |
| 0x8007EAE4 | LIVE | `StartPage::install` | game/ui/start_page.cpp:57 |  |  |
| 0x8007F104 | LIVE | `pageScope<0x8007F104u>` | game/ui/options_page.cpp:121 |  | installed via tomba::native::declareOverride() at game/ui/options_page… |
| 0x8007F250 | LIVE | `pageScope<0x8007F250u>` | game/ui/options_page.cpp:121 |  | installed via tomba::native::declareOverride() at game/ui/options_page… |
| 0x8007F498 | LIVE | `pageScope<0x8007F498u>` | game/ui/options_page.cpp:121 |  | installed via tomba::native::declareOverride() at game/ui/options_page… |
| 0x8007F73C | LIVE | `pageScope<0x8007F73Cu>` | game/ui/options_page.cpp:121 |  | installed via tomba::native::declareOverride() at game/ui/options_page… |
| 0x8007F8F8 | LIVE | `pageScope<0x8007F8F8u>` | game/ui/options_page.cpp:121 |  | installed via tomba::native::declareOverride() at game/ui/options_page… |
| 0x8007FC24 | LIVE | `Render::optionsBackdrop` | game/render/render_options.cpp:72 |  | optionsBackdrop — see render.h. The PICTURE half of FUN_8007FC24 (the … |
| 0x8007FC24 | LIVE | `OptionsPage::pushBackdrop` | game/ui/options_page.cpp:34 |  | ORACLE: guest 0x8007FC24 |
| 0x8007FC24 | LIVE | `OptionsPage::install` | game/ui/options_page.cpp:145 |  |  |
| 0x8007FCC8 | LIVE | `Render::optionsSolidBox` | game/render/render_options.cpp:157 |  | optionsSolidBox — see render.h. Reproduces FUN_8007FCC8(a0=x, a1=y, a2… |
| 0x8007FCC8 | LIVE | `Panel::pushDialogBackdrop` | game/ui/dialog_backdrop.cpp:56 |  | ORACLE: guest 0x8007FCC8 |
| 0x8007FCC8 | LIVE | `ov_push_dialog_backdrop` | game/ui/dialog_backdrop.cpp:90 |  | Guest-ABI entry: x/y/w/h in r4-r7, mode off the caller's stack (see th… |
| 0x8007FCC8 | LIVE | `OptionsPage::noteBox` | game/ui/options_page.cpp:74 |  |  |
| 0x8007FD54 | LIVE | `LoadingText::draw` | game/ui/loading_text.cpp:32 | 0x80079374 | The guest body: blink the palette, draw the string. `mode` (5th arg of… |
| 0x8007FDB0 | LIVE | `Render::submitPolyGt3Native` | game/render/submit.cpp:298 |  | guest 0x8007FDB0 — POLY_GT3 (gouraud-textured triangle) submit. |
| 0x80080114 | LIVE | `Render::submitPolyGt4Native` | game/render/submit.cpp:454 |  | guest 0x8008007C — POLY_GT4 (gouraud-textured quad) submit, PC-NATIVE. |
| 0x800803DC | LIVE | `Render::gt3gt4` | game/render/submit.cpp:641 |  |  |
| 0x80080F6C | LIVE | `Render::drawSync` | game/render/wide_re_libgpu_leaves.cpp:89 |  | guest 0x80080F6C (0x80080F6C) — DrawSync(mode). VERIFIED & WIRED 2026-… |
| 0x80080F6C | LIVE | `ov_drawSync` | game/render/wide_re_libgpu_leaves.cpp:222 |  |  |
| 0x80081218 | LIVE | `Asset::uploadImage` | game/core/asset.cpp:308 |  | DO NOT REGISTER 0x80081218 IN THE OVERRIDE REGISTRY. It surfaces near … |
| 0x80081458 | LIVE | `Render::clearOTagR` | game/render/wide_re_libgpu_leaves.cpp:153 |  | guest 0x80081458 (0x80081458) — ClearOTagR(OT, entries). VERIFIED & WI… |
| 0x80081458 | LIVE | `ov_clearOTagR` | game/render/wide_re_libgpu_leaves.cpp:225 |  |  |
| 0x80081560 | LIVE | `Engine::drawOTag` | game/game_tomba2.cpp:138 |  | Native ownership of DrawOTag (libgpu FUN_80081560, the per-frame draw … |
| 0x800815D0 | LIVE | `nativePutDrawEnv` | game/render/wide_re_gpu_putdrawenv.cpp:265 |  | nativePutDrawEnv (0x800815D0) — libgpu PutDrawEnv(drawEnvPtr). DRAFT. … |
| 0x80081CF8 | LIVE | `buildDrawAreaRect` | game/render/hud_gauge_emitter.cpp:151 |  | ----------------------------------------------------------------------… |
| 0x80081CF8 | LIVE | `emitDrawAreaAndLink` | game/render/hud_gauge_emitter.cpp:163 |  | Emit the DR_AREA packet built from the sp+rectOff rect into the packet… |
| 0x80081FB0 | LIVE | `LibgpuDrawEnv::setDrawEnv` | game/render/libgpu_draw_env.cpp:108 |  | GUEST_ADDRESS: 80081FB0 authenticated executable/overlay evidence |
| 0x80082220 | LIVE | `nativeDrawMode` | game/render/wide_re_gpu_putdrawenv.cpp:184 |  | nativeDrawMode (0x80082220) — DR_TPAGE mode-word builder. DRAFT. RE'd … |
| 0x80082240 | LIVE | `nativeClipTopLeft` | game/render/wide_re_gpu_putdrawenv.cpp:112 |  | nativeClipTopLeft (0x80082240) — SetDrawAreaTopLeft(x,y) word builder.… |
| 0x800822D8 | LIVE | `nativeClipBottomRight` | game/render/wide_re_gpu_putdrawenv.cpp:141 |  | nativeClipBottomRight (0x800822D8) — SetDrawAreaBottomRight(x,y) word … |
| 0x80082370 | LIVE | `nativeDrawOffset` | game/render/wide_re_gpu_putdrawenv.cpp:170 |  | nativeDrawOffset (0x80082370) — SetDrawingOffset(x,y) word builder. DR… |
| 0x8008238C | LIVE | `nativeTextureWindow` | game/render/wide_re_gpu_putdrawenv.cpp:212 |  | nativeTextureWindow (0x8008238C) — DR_TWIN word builder. DRAFT. RE'd f… |
| 0x80082424 | LIVE | `Render::gpuDmaSend` | game/render/wide_re_gpu_dma_queue.cpp:596 |  | guest 0x80082424 (0x80082424) — GpuDmaSend(arrayPtr, count). VERIFIED … |
| 0x80082424 | LIVE | `ov_gpuDmaSend` | game/render/wide_re_gpu_dma_queue.cpp:671 |  |  |
| 0x80082734 | LIVE | `Render::gpuLoadImageStream` | game/render/wide_re_gpu_loadimage_streamer.cpp:135 |  | guest 0x80082734 (0x80082734) — libgpu LoadImage()-internal chunked GP… |
| 0x80082734 | LIVE | `ov_gpuLoadImageStream` | game/render/wide_re_gpu_loadimage_streamer.cpp:276 |  |  |
| 0x80082C68 | LIVE | `libgpuDmaStatusReset` | game/render/wide_re_libgpu_leaves.cpp:257 |  | libgpuDmaStatusReset (0x80082C68) — GPU-DMA status-block RESET. RE-VER… |
| 0x80082D04 | LIVE | `Render::gpuDmaQueueEnqueue` | game/render/wide_re_gpu_dma_queue.cpp:169 |  | guest 0x80082D04 (0x80082D04) — GpuDmaQueueEnqueue(fn, argValOrPtr, si… |
| 0x80082D04 | LIVE | `ov_gpuDmaQueueEnqueue` | game/render/wide_re_gpu_dma_queue.cpp:662 |  |  |
| 0x80082FB4 | LIVE | `Render::gpuDmaQueueDrain` | game/render/wide_re_gpu_dma_queue.cpp:351 |  | guest 0x80082FB4 (0x80082FB4) — GpuDmaQueueDrain(). VERIFIED & WIRED 2… |
| 0x80082FB4 | LIVE | `ov_gpuDmaQueueDrain` | game/render/wide_re_gpu_dma_queue.cpp:665 |  |  |
| 0x80083364 | LIVE | `Render::gpuDmaQueueSync` | game/render/wide_re_gpu_dma_queue.cpp:483 |  | guest 0x80083364 (0x80083364) — GpuDmaQueueSync(mode). VERIFIED & WIRE… |
| 0x80083364 | LIVE | `ov_gpuDmaQueueSync` | game/render/wide_re_gpu_dma_queue.cpp:668 |  |  |
| 0x80083DE0 | LIVE | `libgpuSetDrawMode` | game/render/wide_re_libgpu_leaves.cpp:295 |  | libgpuSetDrawMode (0x80083DE0) — libgpu **SetDrawMode(DR_MODE* p, int … |
| 0x80083E80 | LIVE | `Trig::rsin` | game/math/trig.cpp:4 |  |  |
| 0x80083E80 | LIVE | `Trig::registerOverrides` | game/math/trig.cpp:149 |  | UNREGISTERED (2026-07-15): rsin/ratan2 are NOT safe as overrides. Thei… |
| 0x80083F50 | LIVE | `Trig::rcos` | game/math/trig.cpp:86 |  |  |
| 0x80084080 | LIVE | `Math::sqrtLzc` | game/math/gte_math.cpp:718 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80084110 | LIVE | `Math::matMul` | game/math/gte_math.cpp:123 |  |  |
| 0x80084220 | LIVE | `Math::applyMatlv` | game/math/gte_math.cpp:676 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80084250 | LIVE | `GteTransform3::rotate3AndPackIr` | game/math/wide_re_gte_transform3.cpp:48 |  |  |
| 0x80084360 | LIVE | `Math::matLoadLV` | game/math/gte_math.cpp:749 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80084470 | LIVE | `Math::applyMatrixLV` | game/math/gte_math.cpp:181 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x800844C0 | LIVE | `Math::applyMatrixSV` | game/math/gte_math.cpp:305 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80084520 | LIVE | `Math::matColScale` | game/math/gte_math.cpp:804 |  | ORACLE: guest 0x80084520 |
| 0x800847B0 | LIVE | `vertexHeaderRepack` | game/render/wide_re_libgpu_leaves.cpp:343 |  | vertexHeaderRepack (0x800847B0) — 20-byte SoA->AoS vertex-header REPAC… |
| 0x800847F0 | LIVE | `Math::rotMatSoft` | game/math/gte_math.cpp:508 |  |  |
| 0x80084A80 | LIVE | `Math::rotMatSoftInverse` | game/math/gte_math.cpp:550 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80084D10 | LIVE | `Math::rotX` | game/math/gte_math.cpp:476 |  |  |
| 0x80084EB0 | LIVE | `Math::rotY` | game/math/gte_math.cpp:472 |  |  |
| 0x80085050 | LIVE | `Math::rotZ` | game/math/gte_math.cpp:468 |  |  |
| 0x800851F0 | LIVE | `Math::rotMatSoftYXZ` | game/math/gte_math.cpp:641 |  | ──────────────────────────────────────────────────────────────────────… |
| 0x80085480 | LIVE | `Math::rotmat` | game/math/gte_math.cpp:380 |  |  |
| 0x80085690 | LIVE | `Trig::ratan2` | game/math/trig.cpp:28 |  |  |
| 0x80085C9C | LIVE | `LibapiIntr::setIntrMask` | game/core/libapi_intr.cpp:112 |  |  |
| 0x80086230 | LIVE | `LibapiIntr::initVblankCallbacks` | game/core/libapi_intr.cpp:122 |  | FUN_0x80086230 — VBlank-callback subsystem init: clear the 8-slot VSyn… |
| 0x80086288 | LIVE | `LibapiIntr::runVblankCallbacks` | game/core/libapi_intr.cpp:145 |  | FUN_0x80086288 — the VBlank handler itself: bump the tick counter, the… |
| 0x80086320 | LIVE | `LibapiIntr::clearWords` | game/core/libapi_intr.cpp:175 |  | FUN_0x80086320 — the word-fill helper: writes N words of a constant. |
| 0x80086604 | LIVE | `Engine::activeModeCtx` | game/scene/startup.cpp:337 |  | Engine::activeModeCtx. Accessor: returns the active mode/draw-env cont… |
| 0x80086604 | LIVE | `ov_engineActiveModeCtx` | game/core/engine.cpp:4357 |  | installed via tomba::native::declareOverride() at game/core/engine.cpp… |
| 0x80086620 | LIVE | `eng_init_mode_ctrl` | game/scene/startup.cpp:201 |  | engine MODE control: file-local helper (only called from Engine::initS… |
| 0x80086738 | LIVE | `Engine::installModeHandlers` | game/scene/startup.cpp:346 |  | Engine::installModeHandlers. Installs the mode handler table at 0x8010… |
| 0x80086738 | LIVE | `ov_engineInstallModeHandlers` | game/core/engine.cpp:4360 |  | installed via tomba::native::declareOverride() at game/core/engine.cpp… |
| 0x80086764 | LIVE | `Engine::runModeEnter` | game/scene/startup.cpp:364 |  | Engine::runModeEnter. If both bit0 flags in the mode ctx (*0x800ABE98)… |
| 0x80086764 | LIVE | `ov_engineRunModeEnter` | game/core/engine.cpp:4363 |  | installed via tomba::native::declareOverride() at game/core/engine.cpp… |
| 0x80087A60 | LIVE | `Engine::initInput` | game/scene/startup.cpp:236 | 0x80080890 0x800808A0 0x80085B10 0x800873F0 0x80087400 | a thin wrapper that just calls FUN_80086970; owned as initInput(). |
| 0x80088B00 | LIVE | `Engine::initAlloc` | game/scene/startup.cpp:269 | 0x80086738 0x80089160 0x8009A340 | engine ALLOCATOR / dispatch-table init. `s1` / `s2` are the struct-spa… |
| 0x8008913C | LIVE | `Engine::allocRecordForSelector` | game/scene/startup.cpp:42 |  | returns the base of record[0] or record[1] of the 240-byte-stride, 2-e… |
| 0x8008913C | LIVE | `ov_allocRecordForSelector` | game/scene/startup.cpp:387 |  |  |
| 0x8008A110 | LIVE | `LibcdNative::posToInt` | game/cd/libcd_native.cpp:34 |  |  |
| 0x8008B8F0 | LIVE | `LibcdNative::searchFile` | game/cd/libcd_native.cpp:23 |  |  |
| 0x8008BBE8 | LIVE | `LibcdNative::newMedia` | game/cd/libcd_native.cpp:12 |  |  |
| 0x8008BF50 | LIVE | `LibcdNative::cacheFile` | game/cd/libcd_native.cpp:17 |  |  |
| 0x80090160 | LIVE | `Sequencer::channelStreamAccumulate` | game/audio/sequencer.cpp:1612 |  | channelStreamAccumulate — true leaf (no stack frame). Faithful to gues… |
| 0x800909C0 | LIVE | `Sequencer::frameTick` | game/audio/sequencer.cpp:138 |  | libsnd per-VBlank tick wrapper. WIDE-RE DRAFT, UNWIRED (see header). |
| 0x80090BD0 | LIVE | `Sequencer::seqChannelDispatch` | game/audio/sequencer.cpp:233 | 0x80090E40 0x80091050 0x80091910 0x80092080 | SsSeqCalled — the per-VBlank sequence/channel scheduler. Faithful to |
| 0x80090E40 | LIVE | `Sequencer::channelPitchSlideTick` | game/audio/sequencer.cpp:591 |  | channelPitchSlideTick — pitch-slide/portamento per-tick interpolator (… |
| 0x80091050 | LIVE | `Sequencer::channelReleaseClear` | game/audio/sequencer.cpp:173 |  | "release"/note-off housekeeping: zeroes the per-channel status byte at… |
| 0x800910F0 | LIVE | `Sequencer::channelPitchSelectDispatch` | game/audio/sequencer.cpp:158 |  | thin arg-repacking wrapper: sign-extend (seq,chan) to 32-bit and tail-… |
| 0x80091810 | LIVE | `Sequencer::channelVoiceKeyOn` | game/audio/sequencer.cpp:1678 |  | channelVoiceKeyOn — true leaf (no stack frame). Faithful to guest 0x80… |
| 0x80091910 | LIVE | `Sequencer::channelStopFlagSet` | game/audio/sequencer.cpp:207 |  | sets the per-channel status byte at +20 to 1, clears flags bit3 (value… |
| 0x80091970 | LIVE | `Sequencer::channelNoteInit` | game/audio/sequencer.cpp:987 | 0x800931A0 | channelNoteInit — per-channel note retrigger (SsSeqCalled flags bit2 r… |
| 0x80092080 | LIVE | `Sequencer::channelEnvelopeRampTick` | game/audio/sequencer.cpp:783 |  | channelEnvelopeRampTick — ADSR/envelope ramp (SsSeqCalled flags bit6 A… |
| 0x80092310 | LIVE | `Sequencer::channelToneRecordCopy` | game/audio/sequencer.cpp:1744 |  | channelToneRecordCopy — stack frame present (sp-32, spill r16@16/r17@2… |
| 0x80092420 | LIVE | `Sequencer::channelToneRecordCopyWide` | game/audio/sequencer.cpp:1812 |  | channelToneRecordCopyWide — stack frame present (sp-32, spill r16@16/r… |
| 0x800931C0 | LIVE | `Sequencer::voiceStateFlush` | game/audio/sequencer.cpp:2371 | 0x80097E10 0x80098DB0 0x80098F90 0x80099970 0x8009A1D0 | the sound driver's per-frame SPU voice-state flush. 12,000 substrate d… |
| 0x80094150 | LIVE | `Sequencer::voiceAllocateOrSteal` | game/audio/sequencer.cpp:1935 |  | voiceAllocateOrSteal — true leaf (no stack frame). Faithful to guest 0… |
| 0x80094474 | LIVE | `Sequencer::channelNotePeriodCompute` | game/audio/sequencer.cpp:2183 |  | channelNotePeriodCompute — true leaf (no stack frame). Faithful to gue… |
| 0x80094B50 | LIVE | `Sequencer::channelKeyRegisterMerge` | game/audio/sequencer.cpp:496 |  | channelKeyRegisterMerge — true leaf (no stack frame). Faithful to gues… |
| 0x80095530 | LIVE | `Sequencer::channelVoiceRegisterWrite` | game/audio/sequencer.cpp:1138 |  | channelVoiceRegisterWrite — the "SPU voice-register write leaf" channe… |
| 0x80095A9C | LIVE | `Sequencer::channelVolumeSnapshot` | game/audio/sequencer.cpp:469 |  | channelVolumeSnapshot — true leaf (no stack frame). Faithful to guest … |
| 0x80095B90 | LIVE | `Sequencer::channelKeyEventScan` | game/audio/sequencer.cpp:537 |  | channelKeyEventScan — stack frame present (sp-32, spill ra/s16/s17/s18… |
| 0x800962B0 | LIVE | `Sequencer::channelVoiceSelectPrep` | game/audio/sequencer.cpp:1066 |  | channelVoiceSelectPrep — called mid-loop by channelVoiceRegisterWrite(… |
| 0x80096370 | LIVE | `Font::bank2Store` | game/ui/font.cpp:94 |  | font-bank2 store. `*kFontBank2Addr(sb) = bank; jr ra`. Leaf; does NOT … |
| 0x800963A0 | LIVE | `Font::bankSelect` | game/ui/font.cpp:81 |  | font-bank selector. If ((bank-1)&0xff) < 24, store the bank byte at |
| 0x80096878 | LIVE | `bav_cleanup_tail` | game/ui/bav_loader.cpp:90 |  | cleanup tail at 0x80096878: release lock (a0=0 path) + decrement refco… |
| 0x80099450 | LIVE | `bav_lock_ready` | game/ui/bav_loader.cpp:80 |  | -- lock helpers (FUN_80099478 / FUN_80099450), inlined --- |
| 0x80099478 | LIVE | `bav_lock_ready` | game/ui/bav_loader.cpp:80 |  | -- lock helpers (FUN_80099478 / FUN_80099450), inlined --- |
| 0x800998E4 | LIVE | `AreaSlots::classifySlotStates` | game/world/area_slots.cpp:336 |  | ORACLE: guest 0x800998E4 |
| 0x8009A3E0 | LIVE | `Str::copyBytes` | game/core/str.cpp:37 |  | memcpy(dst, src, n). RE from authenticated executable/overlay evidence… |
| 0x8009A3E0 | LIVE | `ov_copyBytes` | game/core/str.cpp:77 |  |  |
| 0x8009A640 | LIVE | `Str::compareBytes` | game/core/str.cpp:84 |  | FUN_0x8009A640 — byte compare, sibling of the memcpy already owned her… |
| 0x800A33C8 | LIVE | `tbl_strp` | game/ai/beh_cube_text_spawn.cpp:45 |  | string-table entry pointer: mem32(0x800a33c8 + (node[0x60]*3 << 2) + 4… |
| 0x800A6490 | LIVE | `MeshQuads::trig` | game/render/mesh_quads.cpp:87 |  |  |
| 0x800BE224 | LIVE | `MusicCoord::musicFadeIn` | game/audio/music_coord.cpp:48 |  | PC-added helper (NOT a port of any FUN_XXXX): snap the game's CD-volum… |
| 0x800BED80 | LIVE | `MusicCoord::dialogToneActive` | game/audio/music_coord.cpp:34 |  |  |
| 0x800BF842 | LIVE | `Engine::postRenderTick` | game/core/engine.cpp:3481 |  | Engine::postRenderTick — 3-state fx-trigger + countdown on byte 0x800B… |
| 0x800BF9B4 | LIVE | `Render::worldVoidBeat` | game/render/render_walk.cpp:422 |  | Per-frame WORLD-pass gates (render.h): one definition each, read by BO… |
| 0x800EE489 | LIVE | `Cull::cullFarMult` | game/render/cull.cpp:100 |  | pc_faithful/native_sync split (2026-07-03): pc_faithful (native_sync=f… |
| 0x800F2418 | LIVE | `Render::areaCacheTrustTick` | game/render/render_walk.cpp:467 |  | AREA-SCOPED CACHE trust latches (see render.h mSceneTableTrusted/mBack… |
| 0x800F2624 | LIVE | `Render::terrainRenderAll` | game/render/submit.cpp:808 |  | terrainRenderAll: the terrain-node enumeration (moved from render_walk… |
| 0x80104368 | LIVE | `cdlibcd_read_into_scratch` | game/core/engine.cpp:3961 |  | Read one 2048 B disc sector into a local buffer AND into the guest-RAM |
| 0x801062E4 | LIVE | `Render::renderAttract` | game/render/render_attract.cpp:92 |  | #6 DEMO/TITLE ATTRACT (stage 0x801062E4, sm[0x48]==7): the live 3D fie… |
| 0x801062E4 | LIVE | `Render::renderTitle` | game/render/render_walk.cpp:214 |  | #2 DEMO/TITLE front-end (stage 0x801062E4). Substate s2 (sm[0x48]==2) … |
| 0x801062E4 | LIVE | `Render::titleNative` | game/render/render_walk.cpp:398 |  | titleNative — see render.h. Read-only producer for the DEMO/title fron… |
| 0x801062E4 | LIVE | `Demo::stageMain` | game/scene/demo.cpp:553 | 0x800810F0 | DEMO stage entry (0x801062E4) — own the prologue PC-native, then hand … |
| 0x801062E4 | LIVE | `Demo::stageBodyFaithful` | game/scene/demo.cpp:1084 | 0x8001CF00 0x80044BD4 0x80045080 0x8005082C 0x80051F80 0x80052078 … |  |
| 0x8010637C | LIVE | `Engine::stagePrologue` | game/core/engine.cpp:3035 |  | GAME stage TOP-LEVEL ENTRY 0x8010637C — task-0's stage driver: a one-t… |
| 0x8010637C | ORPHAN | `Engine::stageBodyFaithful` | game/core/engine.cpp:3071 | 0x80051F80 0x801086E0 0x80108720 0x80108784 | pc_faithful GAME stage body (fiber task; see engine.h). Byte shape: |
| 0x801063C0 | LIVE | `Demo::s0` | game/scene/demo.cpp:396 |  | s0 0x801063C0 — run-once INIT then loaders; FALLS THROUGH into s1 same… |
| 0x801063F4 | LIVE | `Engine::frame` | game/core/engine.cpp:2973 |  | One native loop iteration of the guest body 0x801063F4: dispatch sm[0x… |
| 0x801063F4 | ORPHAN | `Engine::stageMain` | game/core/engine.cpp:3104 |  | OLD guest-loop entry (prologue + guest-continuation into the guest loo… |
| 0x8010641C | LIVE | `Demo::s1` | game/scene/demo.cpp:74 | 0x80106F80 | s1 0x8010641C — wait/advance: v0 = inner menu input machine 0x80106f80… |
| 0x80106464 | LIVE | `Demo::s2` | game/scene/demo.cpp:96 | 0x8001CF2C 0x8010696C | s2 0x80106464 — sub-machine v0 = 0x8010696c(). Outcome 1 -> go to s7 (… |
| 0x80106478 | LIVE | `Engine::areaLoadState` | game/core/engine.cpp:243 | 0x8001CF2C 0x8004D8B0 0x80078824 0x8007BF20 0x8007E8DC 0x8007ED5C … | Engine::areaLoadState — native ownership of FUN_80106478 (the |
| 0x8010649C | LIVE | `native_stage0_sm` | game/core/engine.cpp:3884 |  | Stage-0 START.BIN state machine (overlay 0x80106728), PC-native. guest… |
| 0x8010649C | LIVE | `Render::renderStartBoot` | game/render/render_walk.cpp:206 |  | #1 START.BIN boot (0x8010649C): the loader shows a black screen (empty… |
| 0x801064E8 | LIVE | `Demo::s3` | game/scene/demo.cpp:134 | 0x800750D8 0x80106AC4 | s3 0x801064E8 — sub-machine v0 = 0x80106ac4() (mirror of 0x8010696c). … |
| 0x80106580 | LIVE | `load_machine_s4` | game/scene/demo.cpp:836 | 0x8001CF2C 0x800750D8 0x8007BE18 | Substate s4 (0x80106580) — LOAD GAME. The body runs the load sub-machi… |
| 0x801065DC | LIVE | `demo_frame_s5` | game/scene/demo.cpp:819 |  | Substate s5 (0x801065DC) — LEAVE DEMO: the body is `jal 0x80052078(2)`… |
| 0x801065EC | LIVE | `Demo::s6` | game/scene/demo.cpp:330 | 0x8007B45C 0x80106690 0x80106824 | s6 0x801065EC — page sub-machine 0x8007b45c(); if sm[0x50]==3 fire the… |
| 0x80106690 | LIVE | `Render::menuChrome` | game/render/render_walk.cpp:325 |  | menuChrome — see render.h. The black backdrop + the 2 logo sprites (FU… |
| 0x80106728 | LIVE | `native_stage0_sm` | game/core/engine.cpp:3884 |  | Stage-0 START.BIN state machine (overlay 0x80106728), PC-native. guest… |
| 0x80106824 | LIVE | `Render::optionsPageNative` | game/render/render_options.cpp:204 |  | optionsPageNative — see render.h. The page ITSELF is produced at its g… |
| 0x80106824 | LIVE | `Render::menuItemsAndCursor` | game/render/render_walk.cpp:376 |  | menuItemsAndCursor — see render.h. Reproduces FUN_80106824(param1, par… |
| 0x80106824 | LIVE | `Render::s3MenuNative` | game/render/render_walk.cpp:408 |  | s3MenuNative — see render.h. The page-1 menu (sm[0x48]==3, reached by … |
| 0x8010696C | LIVE | `Demo::s2SubMachine` | game/scene/demo.cpp:243 | 0x80106690 0x80106824 |  |
| 0x80106AC4 | LIVE | `Demo::s3SubMachine` | game/scene/demo.cpp:166 | 0x80106690 0x80106824 |  |
| 0x80106AC4 | LIVE | `Demo::registerOverrides` | game/scene/demo.cpp:322 |  |  |
| 0x80106B98 | LIVE | `Engine::fieldRunFaithful` | game/core/engine.cpp:1543 | 0x8001CF2C 0x800263E8 0x80045580 0x8005082C 0x80050894 0x800508A8 … | FIELD RUNNING sub-machine 0x80106b98 — native control flow + state bod… |
| 0x80106C24 | LIVE | `Render::attractItemLive` | game/render/render_attract.cpp:84 |  | attractItemLive — IS THERE AN ATTRACT WORLD THIS FRAME? (kanban #86, t… |
| 0x80106F80 | LIVE | `demo_menu_machine` | game/scene/demo.cpp:605 | 0x8001CF00 0x8008CCE0 0x8008CD40 0x8009C820 0x8009C8BC 0x80106F80 | s1's inner menu input machine (0x80106F80): an 8-way state machine on … |
| 0x801070B4 | LIVE | `Engine::fieldRunXFaithful` | game/core/engine.cpp:2705 | 0x8005082C 0x80050894 0x8006C77C | FIELD RUNNING sub-machine VARIANT 0x801070b4 (sm[0x4c]==3, the mid-tra… |
| 0x80107AFC | LIVE | `Engine::transitionMain` | game/core/engine.cpp:2119 |  | the MAIN door/sub-scene transition (sm[0x4c]==1..4). sm[0x4e]: |
| 0x80107AFC | LIVE | `Engine::transitionMainFaithful` | game/core/engine.cpp:2396 | 0x8003FA1C 0x80044BD4 0x80050894 0x80059C60 0x8006EF38 0x80074E48 … | Faithful mirror of overlay guest 0x80107AFC. Frame: sp-=24, r31 spill … |
| 0x80107D3C | LIVE | `Engine::transitionD3c` | game/core/engine.cpp:2188 |  | transition variant (sm[0x4c]==5/6). sm[0x4e]: 0 load, 1 effect |
| 0x80107D3C | LIVE | `Engine::transitionD3cFaithful` | game/core/engine.cpp:2498 | 0x8003EA88 0x8003FB84 0x80044BD4 | Faithful mirror of overlay guest 0x80107D3C. Frame: sp-=24, r16 spill … |
| 0x80107E20 | LIVE | `Engine::transitionE20` | game/core/engine.cpp:2214 |  | transition variant (sm[0x4c]==7). sm[0x4e]: 0 setup+load, 1 |
| 0x80107E20 | LIVE | `Engine::transitionE20Faithful` | game/core/engine.cpp:2540 | 0x8003E264 0x8003E894 0x80044BD4 0x80074BF8 0x80074E48 | Faithful mirror of overlay guest 0x80107E20. Frame: sp-=32, |
| 0x80107F3C | LIVE | `Engine::transitionF3c` | game/core/engine.cpp:2246 |  | transition variant (sm[0x4c]==8), a 7-state machine. NB case 0 |
| 0x80107F3C | LIVE | `Engine::transitionF3cFaithful` | game/core/engine.cpp:2594 | 0x8001CF2C 0x8003E264 0x8003E894 0x8003EBE0 0x8003FB94 0x80044BD4 … | Faithful mirror of overlay guest 0x80107F3C. Frame: sp-=24, r31 spill … |
| 0x8010810C | LIVE | `Engine::submitPage810c` | game/core/engine.cpp:497 | 0x801084F8 | page-1 dim-fade branch (task+0x6B == 1, "draw main pause menu" — |
| 0x8010810C | LIVE | `Engine::submitPage810cFaithful` | game/core/engine.cpp:526 | 0x8007E9C8 0x801084F8 | pc_faithful mirror of overlay guest 0x8010810C's page-1 (pause-menu di… |
| 0x801086E0 | LIVE | `Engine::stageAreaInit` | game/core/engine.cpp:153 |  | sm[0x48] == 0 — area INIT: advance to running (sm[0x48]=2), reset the |
| 0x80108720 | LIVE | `Engine::stageResumeInit` | game/core/engine.cpp:173 |  | sm[0x48] == 1 — area RESUME-INIT (re-enter a running area, sub-mode 1)… |
| 0x8010882C | LIVE | `Engine::stageRunning` | game/core/engine.cpp:576 |  | sm[0x48]==2 RUNNING, per-frame variant: dispatch sm[0x4a] handler. han… |
| 0x8010882C | LIVE | `Engine::submode0` | game/core/engine.cpp:628 | 0x80109450 | GAME sub-mode-0 bridge 0x8010882c (sm[0x4c]/sm[0x4e] dispatch) — nativ… |
| 0x801088D8 | LIVE | `Engine::submode1Faithful` | game/core/engine.cpp:2853 | 0x80044BD4 0x8005245C 0x80107230 0x8010766C 0x80107790 | pc_faithful walkable-field area machine — mirror of overlay guest 0x80… |
| 0x80108A60 | LIVE | `Engine::fieldTransition` | game/core/engine.cpp:2311 |  | sm[0x4a]==5 transition dispatcher on sm[0x4c]. 0/9 = done |
| 0x80108A60 | LIVE | `Engine::fieldTransitionFaithful` | game/core/engine.cpp:2348 |  | Faithful mirror of overlay guest 0x80108A60. Own frame: sp-=24, r31 sp… |
| 0x80108B0C | LIVE | `Engine::devTeleportApply` | game/core/engine.cpp:1026 |  | FIELD PER-FRAME UPDATE 0x80108b0c — native control flow (the field fra… |
| 0x80108B0C | LIVE | `Engine::fieldFrameFaithful` | game/core/engine.cpp:1040 | 0x80075A80 |  |
| 0x80108BE4 | LIVE | `Engine::fieldFrameXFaithful` | game/core/engine.cpp:2010 |  | FIELD PER-FRAME UPDATE VARIANT 0x80108be4 — the mid-TRANSITION field f… |
| 0x80109164 | LIVE | `Sop::areaLoad` | game/scene/sop.cpp:88 | 0x8001DC40 | Owned synchronous area-DATA load (replaces the body of LAB_80109164 |
| 0x80109164 | LIVE | `Sop::areaLoadFaithful` | game/scene/sop.cpp:887 | 0x8001DC40 0x80044E84 | pc_faithful SOP area-load task body — mirror of overlay guest 0x801091… |
| 0x801092B4 | LIVE | `Sop::fieldUpdate` | game/scene/sop.cpp:524 |  | SOP per-frame FIELD UPDATE — native ownership of FUN_801092b4 (decomp |
| 0x80109450 | LIVE | `Render::renderSopNarration` | game/render/render_walk.cpp:273 |  | #5 SOP INTRO NARRATION (overlay-sig 0x3C021F80 @ 0x80109450): the WORL… |
| 0x80109450 | LIVE | `Sop::fieldMode` | game/scene/sop.cpp:609 |  | SOP FIELD-MODE MACHINE — native ownership of FUN_80109450 (decomp |
| 0x80109450 | LIVE | `Sop::fieldModeFaithful` | game/scene/sop.cpp:728 | 0x8001CF2C 0x80044BD4 0x8006CBD0 0x8006E3B0 0x80075240 0x80078610 … |  |
| 0x8010957C | LIVE | `ScreenFade::sequence` | game/render/screen_fade.cpp:113 | 0x8010CC68 0x8010D030 |  |
| 0x80109FE0 | LIVE | `Render::fieldEntityRender` | game/render/submit.cpp:690 |  | FIELD ENTITY RENDER LOOP — PC-native ownership of the SOP field-overla… |
| 0x8010A0E0 | LIVE | `Sop::scenePrepass` | game/scene/sop.cpp:451 |  | SOP scene cam-frustum prepass — native ownership of FUN_8010A0E0 (Ghid… |
| 0x8010A3AC | LIVE | `Sop::sceneGridGather` | game/scene/sop.cpp:317 |  | sceneGridGather — native port of guest FUN_8010A3AC (Ghidra decomp |
| 0x8010AB38 | LIVE | `beh_sop_overlay_shadow` | game/ai/sop_overlay_shadow.cpp:80 |  |  |
| 0x8010ACFC | LIVE | `beh_sop_intro_pilot` | game/ai/beh_sop_intro_pilot.cpp:120 |  |  |
| 0x8010AE30 | LIVE | `native_sop_overlay_shadow_spawn` | game/ai/sop_overlay_shadow.cpp:62 |  | (parent) -> node ptr (0 on pool exhaustion). |
| 0x8010AF60 | LIVE | `sopBeatAdvanceWalk` | game/ai/sop_intro_events.cpp:71 |  | ======================================================================… |
| 0x8010B078 | LIVE | `sopBeatAdvanceNarration` | game/ai/sop_intro_events.cpp:135 |  | ======================================================================… |
| 0x8010B11C | LIVE | `sopOrbitPathStep` | game/ai/sop_intro_events.cpp:188 | 0x80077C40 | ======================================================================… |
| 0x8010B2D4 | LIVE | `sopIntroEffectTick` | game/ai/sop_intro_events.cpp:326 | 0x800519E0 0x8007778C 0x80077C40 | ======================================================================… |
| 0x8010B44C | LIVE | `sopIntroEffectSpawn` | game/ai/sop_intro_events.cpp:277 |  | ======================================================================… |
| 0x8010B588 | LIVE | `overlay_subtick` | game/ai/beh_sop_intro_lifted.cpp:70 | 0x8010B588 | (sopLiftedSubtick, sop_intro_events.cpp) is VERIFIED + WIRED (2026-07-… |
| 0x8010B588 | LIVE | `sopLiftedSubtick` | game/ai/sop_intro_events.cpp:542 |  | GUEST FRAME (2026-07-10 §9 fix): overlay guest 0x8010B588 pushes `addi… |
| 0x8010B798 | LIVE | `beh_sop_intro_lifted` | game/ai/beh_sop_intro_lifted.cpp:118 |  |  |
| 0x8010B990 | LIVE | `beh_sop_intro_narration` | game/ai/beh_sop_intro_narration.cpp:154 |  |  |
| 0x8010BB64 | LIVE | `Render::area21SkyGradientRender` | game/render/area21_sky_gradient.cpp:45 |  |  |
| 0x8010BEAC | LIVE | `beh_orbit_spark_effect` | game/ai/sop_intro_events.cpp:581 |  | ======================================================================… |
| 0x8010C7F4 | LIVE | `Render::fxParticleFieldRender` | game/render/fx_sprite.cpp:986 |  |  |
| 0x8010E258 | LIVE | `ActorObjectContact::resolveHitOrProximity` | game/ai/actor_object_contact.cpp:115 |  | ORACLE: overlay guest 0x8010E258 |
| 0x8010E904 | LIVE | `ActorTomba::postFrameWaterCheck` | game/player/actor_tomba.cpp:597 |  | ======================================================================… |
| 0x8010EA80 | LIVE | `ActorBump::respondToContact` | game/ai/actor_bump.cpp:100 |  | ORACLE: overlay guest 0x8010EA80 |
| 0x801104D0 | LIVE | `Render::fxBackdropSparkRender` | game/render/fx_backdrop_plane.cpp:252 |  | ── FUN_801104D0 (A0E overlay, area 14) — the backdrop's SPARK/DROPLET … |
| 0x80110C14 | LIVE | `Render::fxRingSpriteRender` | game/render/fx_sprite.cpp:860 |  |  |
| 0x80110CA4 | LIVE | `Render::fxBackdropPlaneRender` | game/render/fx_backdrop_plane.cpp:104 |  |  |
| 0x801110BC | LIVE | `Render::fxDotFieldRender` | game/render/fx_dotfield.cpp:75 |  |  |
| 0x801113B4 | LIVE | `Render::fxMotionTrailRender` | game/render/fx_trail.cpp:87 |  | One joint = four quads. In all of them the two "far" vertices are BLAC… |
| 0x80112188 | LIVE | `ActorMeleeEngage::doIt` | game/ai/actor_melee_engage.cpp:30 | 0x80022C78 0x80055844 0x80084080 |  |
| 0x80112188 | LIVE | `ActorMeleeEngage::registerOverrides` | game/ai/actor_melee_engage.cpp:334 |  |  |
| 0x80112A60 | LIVE | `aux_list_walk` | game/ai/area_seaside_perframe.cpp:73 |  | Walk the aux render list, dispatching FUN_80112A60(item) per item type… |
| 0x801130C4 | LIVE | `ActorTomba::postInteractWalk` | game/player/actor_tomba.cpp:441 |  | ======================================================================… |
| 0x80113628 | LIVE | `Render::fieldHudMinimap` | game/render/minimap.cpp:98 |  | (area mode 2) / FUN_801140A0 (area mode 7) — the overlay-resident mini… |
| 0x80113768 | LIVE | `Render::fxCuedSpriteRender` | game/render/fx_sprite.cpp:783 |  | (A0A overlay, area 10) — surfaced by the 22-AREA nofx sweep, which no … |
| 0x80113C5C | LIVE | `Behaviors::areaSeasidePerframe` | game/ai/area_seaside_perframe.cpp:102 | 0x8002288C |  |
| 0x801140A0 | LIVE | `Render::fieldHudMinimap` | game/render/minimap.cpp:98 |  | (area mode 2) / FUN_801140A0 (area mode 7) — the overlay-resident mini… |
| 0x801143C4 | LIVE | `Render::a0fVortexRender` | game/render/fx_vortex.cpp:98 |  | area 15's portal render fn (A0F overlay), rebuilt natively. |
| 0x80114E74 | LIVE | `ActorTomba::type4GuardedCheck` | game/player/actor_tomba.cpp:377 |  | type-4 guarded proximity. |
| 0x8011534C | LIVE | `TileGridLayer::scrollStep` | game/render/tile_grid_layer.cpp:165 |  |  |
| 0x80115598 | LIVE | `Render::backdropRender` | game/render/backdrop.cpp:166 |  | NATIVE BACKDROP tilemap drawer — overlay FUN_80115598 (the seaside fie… |
| 0x80115598 | LIVE | `TileGridLayer::emit` | game/render/tile_grid_layer.cpp:235 | 0x80083DE0 |  |
| 0x80116904 | LIVE | `Render::fxMoteStreakRender` | game/render/fx_motes.cpp:100 |  |  |
| 0x80117658 | LIVE | `beh_prng_velocity_machine` | game/ai/beh_prng_velocity_machine.cpp:568 |  |  |
| 0x801178A4 | LIVE | `whiteFlashPhaseRamp` | game/ai/beh_a06_multi_actor.cpp:60 |  | the 5-phase white-flash phase ramp SM. See the file header for the pha… |
| 0x80117AAC | LIVE | `whiteFadeHold` | game/ai/beh_a06_multi_actor.cpp:139 |  | 3-state fade-hold-fade-back companion to whiteFlashPhaseRamp. |
| 0x80118240 | LIVE | `beh_typed_init_exit_poker` | game/ai/beh_typed_init_exit_poker.cpp:54 |  |  |
| 0x80118690 | LIVE | `shared_8690` | game/ai/beh_typed_init_exit_poker.cpp:44 |  | Shared block @0x80118690: FUN_80051D90(node[0x10], a1_buf, 0x1F8000C0)… |
| 0x801189E8 | LIVE | `beh_a06_multi_actor` | game/ai/beh_a06_multi_actor.cpp:638 |  | The guest body 0x801189E8 is ONE function that descends sp by 32 and s… |
| 0x80118B10 | LIVE | `AssemblyRider::rideSlotAndReactToStroke` | game/ai/assembly_rider.cpp:167 |  | ORACLE: overlay guest 0x80118B10 |
| 0x8011C164 | LIVE | `beh_typed_variant_router` | game/ai/beh_typed_variant_router.cpp:110 |  |  |
| 0x8011CBD0 | LIVE | `beh_node3_router` | game/ai/beh_node3_router.cpp:37 |  |  |
| 0x8011D578 | LIVE | `beh_variant_actor_sm` | game/ai/beh_variant_actor_sm.cpp:49 |  |  |
| 0x8011D988 | LIVE | `beh_actor_move_sm` | game/ai/beh_actor_move_sm.cpp:57 |  |  |
| 0x80121978 | LIVE | `beh_id_routed_dispatch` | game/ai/beh_id_routed_dispatch.cpp:120 |  |  |
| 0x80122974 | LIVE | `Render::tetherLineRender` | game/render/fx_line.cpp:508 |  | the TETHER: one rope from this object to an anchor chosen by node+0x47… |
| 0x80122BF4 | LIVE | `beh_id_routed_offset_point` | game/ai/beh_id_routed_dispatch.cpp:68 | 0x800844C0 | FUN_0x80122BF4 — keeps a point pinned 119 world units ABOVE a linked o… |
| 0x80123E9C | LIVE | `ReleaseTriggerMotion::hoverBobCycle` | game/ai/release_trigger_motion.cpp:73 | 0x80077B5C | ----------------------------------------------------------------------… |
| 0x801241BC | LIVE | `ReleaseTriggerMotion::leaderFollowSync` | game/ai/release_trigger_motion.cpp:144 | 0x80051D90 0x80123C94 0x8012400C | ----------------------------------------------------------------------… |
| 0x80124328 | LIVE | `ReleaseTriggerMotion::xSweepCycle` | game/ai/release_trigger_motion.cpp:575 |  | a per-frame X-sweep cycle on the release-trigger object. 6,355 substra… |
| 0x801244E8 | LIVE | `ReleaseTriggerMotion::driftReposition` | game/ai/release_trigger_motion.cpp:203 | 0x80051794 0x80077B5C 0x80084360 0x800847F0 0x80124328 | ----------------------------------------------------------------------… |
| 0x801246B4 | LIVE | `ReleaseTriggerMotion::arcSwoopMotion` | game/ai/release_trigger_motion.cpp:269 | 0x80077B5C | ----------------------------------------------------------------------… |
| 0x801249D4 | LIVE | `ReleaseTriggerMotion::doubleArcMotion` | game/ai/release_trigger_motion.cpp:383 | 0x80077B5C | ----------------------------------------------------------------------… |
| 0x80124C6C | LIVE | `ReleaseTriggerMotion::circleOrbitMotion` | game/ai/release_trigger_motion.cpp:479 | 0x80077B5C | ----------------------------------------------------------------------… |
| 0x80124E74 | LIVE | `beh_jumptable_release_trigger` | game/ai/beh_jumptable_release_trigger.cpp:134 | 0x8004B0D8 0x8004DAEC 0x80051D90 0x80077B5C 0x80123E9C 0x801241BC … |  |
| 0x80125E0C | LIVE | `beh_pure_substate_dispatch` | game/ai/beh_pure_substate_dispatch.cpp:40 |  |  |
| 0x80125FE0 | LIVE | `TiltFollower::applyHalvedOwnerPartPitch` | game/ai/tilt_follower.cpp:90 |  | ORACLE: overlay guest 0x80125FE0 |
| 0x80127420 | LIVE | `beh_arm_countdown_if_linked_ready_80127420` | game/ai/beh_toy_spawn_family.cpp:95 |  | (obj) — arm a 20-frame countdown if the linked object (obj[+0x10]'s ta… |
| 0x801274BC | LIVE | `beh_distance_band_predicate_801274bc` | game/ai/beh_toy_spawn_family.cpp:113 |  | (obj) — a distance-band predicate. `row` is looked up from a per-slot … |
| 0x80127510 | LIVE | `beh_spawn_toy_child_type2_80127510` | game/ai/beh_toy_spawn_family.cpp:212 |  | (owner, subtype) — spawn a child whose sub-behavior is picked by `subt… |
| 0x8012763C | LIVE | `beh_spawn_toy_child_type4_8012763c` | game/ai/beh_toy_spawn_family.cpp:165 | 0x8004D650 | (owner) — spawn a type-4 companion child, then feed GBASE's mode byte … |
| 0x80127720 | LIVE | `beh_spawn_toy_child_type5_80127720` | game/ai/beh_toy_spawn_family.cpp:135 |  | (owner) — spawn a type-5 companion child via the legacy allocator, no … |
| 0x80127798 | LIVE | `beh_area_transition_machine` | game/ai/beh_area_transition_machine.cpp:198 | 0x80041194 |  |
| 0x80127C58 | LIVE | `cutsceneDirector` | game/ai/beh_a08_scene_actor.cpp:196 | 0x80081218 0x8013DD48 | ── FUN_80127C58 — the 10-state cutscene director ─────────────────────… |
| 0x80127C9C | LIVE | `dat_tail` | game/ai/beh_area_transition_machine.cpp:76 |  |  |
| 0x80127CD0 | LIVE | `cd0_tail` | game/ai/beh_area_transition_machine.cpp:70 |  |  |
| 0x801280D0 | LIVE | `beh_a08_scene_actor` | game/ai/beh_a08_scene_actor.cpp:786 |  |  |
| 0x801281B8 | LIVE | `RopeSwing::swingTickAndBendSegments` | game/ai/rope_swing.cpp:90 |  | ORACLE: overlay guest 0x801281B8 |
| 0x80128760 | LIVE | `beh_linked_advance_branch` | game/ai/beh_linked_advance_branch.cpp:39 |  |  |
| 0x80129C00 | LIVE | `beh_anim_trigger_gates` | game/ai/beh_anim_trigger_gates.cpp:44 |  |  |
| 0x8012A0B8 | LIVE | `beh_box_seed_phase_gate` | game/ai/beh_box_seed_phase_gate.cpp:46 |  |  |
| 0x8012D27C | LIVE | `SwaySchedule::advanceRateThenSway` | game/ai/sway_schedule.cpp:134 |  | ORACLE: overlay guest 0x8012D27C |
| 0x8012D404 | LIVE | `beh_cull_tick_render` | game/ai/beh_cull_tick_render.cpp:48 |  |  |
| 0x8012D4EC | LIVE | `beh_jumptable_flag_gate` | game/ai/beh_jumptable_flag_gate.cpp:123 |  |  |
| 0x8012D6AC | LIVE | `step_node18` | game/ai/beh_jumptable_flag_gate.cpp:71 |  | LAB_8012d7d8 (and its inline copy at 0x8012d6ac): node[0x18,0x19,0x1a]… |
| 0x8012D78C | LIVE | `advance_node32` | game/ai/beh_jumptable_flag_gate.cpp:85 |  | LAB_8012d78c: node[0x32]+=4; if (int16)node[0x32] < -0x64e -> tail_set… |
| 0x8012D7D8 | LIVE | `step_node18` | game/ai/beh_jumptable_flag_gate.cpp:71 |  | LAB_8012d7d8 (and its inline copy at 0x8012d6ac): node[0x18,0x19,0x1a]… |
| 0x8012D7FC | LIVE | `despawn_flag_block` | game/ai/beh_jumptable_flag_gate.cpp:56 |  | LAB_8012d82c..8012d840: set bit (4 if node[3]==0 else 8) in DAT_800bf9… |
| 0x8012D844 | LIVE | `tail_set1_and_render` | game/ai/beh_jumptable_flag_gate.cpp:48 |  | LAB_8012d844: v0=1; fall into 8012d848 (node[1]=1); then 8012d84c (jal… |
| 0x8012D848 | LIVE | `tail_set1_and_render` | game/ai/beh_jumptable_flag_gate.cpp:48 |  | LAB_8012d844: v0=1; fall into 8012d848 (node[1]=1); then 8012d84c (jal… |
| 0x8012D84C | LIVE | `tail_set1_and_render` | game/ai/beh_jumptable_flag_gate.cpp:48 |  | LAB_8012d844: v0=1; fall into 8012d848 (node[1]=1); then 8012d84c (jal… |
| 0x8012D9E8 | LIVE | `Render::fxRotSpriteTailRender` | game/render/fx_sprite.cpp:747 |  | 's SPRITE TAIL. This controller is two emitters in one function: a lar… |
| 0x8012DA04 | LIVE | `beh_typed_anim_spawn` | game/ai/beh_typed_anim_spawn.cpp:44 |  |  |
| 0x8012E868 | LIVE | `Render::fxAltAnimSpriteRender` | game/render/fx_sprite.cpp:694 |  | (A01 overlay) — the animation-script member of the family. Same shape … |
| 0x8012EB54 | LIVE | `beh_substate_edge_orchestrator` | game/ai/beh_substate_edge_orchestrator.cpp:46 | 0x8012E8A8 0x8012ED84 0x8012F494 0x8012F5B4 0x8012FD88 0x80130524 … |  |
| 0x801316CC | LIVE | `SubstateEdgeLeaves::tickChildOscillators` | game/ai/substate_edge_native.cpp:19 | 0x80130D5C |  |
| 0x80131D08 | LIVE | `beh_two_child_steer` | game/ai/beh_two_child_steer.cpp:47 |  |  |
| 0x80132400 | LIVE | `beh_single_child_cull` | game/ai/beh_single_child_cull.cpp:43 |  |  |
| 0x8013259C | LIVE | `beh_cull_substate_orchestrator` | game/ai/beh_cull_substate_orchestrator.cpp:51 | 0x8013272C 0x80132954 0x80132A88 0x80132D58 0x80132EDC 0x80133184 … |  |
| 0x80133C14 | LIVE | `beh_typed_table_seed_gate` | game/ai/beh_typed_table_seed_gate.cpp:309 |  |  |
| 0x80133D6C | LIVE | `beh_twin_record_steer` | game/ai/beh_twin_record_steer.cpp:69 |  |  |
| 0x80134FD8 | LIVE | `beh_multi_record_phase_machine` | game/ai/beh_multi_record_phase_machine.cpp:65 |  |  |
| 0x801353C8 | LIVE | `common_tail` | game/ai/beh_multi_record_phase_machine.cpp:49 |  | COMMON TAIL (0x801353C8): node[8]++ / FUN_800517F8(node) / node[8]--. |
| 0x80135D64 | LIVE | `beh_quad_record_table_seed` | game/ai/beh_quad_record_table_seed.cpp:51 |  |  |
| 0x801360F4 | LIVE | `Spawn::spawnTypedChild` | game/world/spawn.cpp:453 |  | TYPED-CHILD SPAWN wrappers (A00 overlay, |
| 0x801360F4 | LIVE | `Spawn::spawnQuadRecordChild` | game/world/spawn.cpp:468 |  |  |
| 0x80136158 | LIVE | `beh_sine_motion_sfx` | game/ai/beh_sine_motion_sfx.cpp:55 | 0x8004766C 0x80048750 |  |
| 0x801365C4 | LIVE | `Render::ropeStripRender` | game/render/fx_rope_strip.cpp:123 |  | ropeStripRender — FUN_801365C4's picture. Read-only; emits world quads… |
| 0x801365C4 | LIVE | `ov_ropeStrip` | game/render/fx_rope_strip.cpp:274 | 0x801365C4 | The override body: the guest's own emission first (untouched — psx_ren… |
| 0x80136954 | LIVE | `beh_event_record_machine` | game/ai/beh_event_record_machine.cpp:62 |  |  |
| 0x80136D9C | LIVE | `beh_pure_inner_dispatch` | game/ai/beh_pure_inner_dispatch.cpp:38 |  |  |
| 0x801389C8 | LIVE | `AssemblyCompanion::composeRigAndApplyPartScales` | game/ai/assembly_companion.cpp:253 |  | AssemblyCompanion::composeRigAndApplyPartScales, guest FUN_801389C8 — … |
| 0x80138A64 | LIVE | `AssemblyCompanion::endCamHoldAndRearmOnStroke` | game/ai/assembly_companion.cpp:142 |  | ORACLE: overlay guest 0x80138A64 |
| 0x80138FC8 | LIVE | `beh_typed_jumptable_pair` | game/ai/beh_typed_jumptable_pair.cpp:69 | 0x8004ED94 0x80138B04 0x80138C70 |  |
| 0x801395C0 | LIVE | `beh_sibling_angle_track` | game/ai/beh_sibling_angle_track.cpp:61 |  |  |
| 0x80139728 | LIVE | `beh_a06_fade_flash_ramp_80139728` | game/ai/beh_a06_script_fades.cpp:100 |  |  |
| 0x80139838 | LIVE | `Spawn::spawnTypedChild` | game/world/spawn.cpp:453 |  | TYPED-CHILD SPAWN wrappers (A00 overlay, |
| 0x80139838 | LIVE | `Spawn::spawnSiblingAngleChild` | game/world/spawn.cpp:471 |  |  |
| 0x80139A28 | LIVE | `variant4Phase3` | game/ai/beh_a06_scripted_actor.cpp:140 |  | ── FUN_80139A28 — variant-4 case-3 sub-machine (inner script cycle) ──… |
| 0x80139C84 | LIVE | `sub801398E4` | game/ai/beh_a06_scripted_actor.cpp:236 |  | ── FUN_80139C84 — variant-4 outer sub-machine (5 states) ─────────────… |
| 0x8013A330 | LIVE | `beh_lift_platform` | game/ai/beh_lift_platform.cpp:59 |  |  |
| 0x8013A730 | LIVE | `Spawn::spawnTypedChild` | game/world/spawn.cpp:453 |  | TYPED-CHILD SPAWN wrappers (A00 overlay, |
| 0x8013A730 | LIVE | `Spawn::spawnLiftPlatformChild` | game/world/spawn.cpp:477 |  |  |
| 0x8013A900 | LIVE | `beh_child_trig_motion` | game/ai/beh_child_trig_motion.cpp:60 |  |  |
| 0x8013AA14 | LIVE | `beh_a06_scripted_actor` | game/ai/beh_a06_scripted_actor.cpp:506 |  |  |
| 0x8013AC34 | LIVE | `Spawn::spawnTypedChild` | game/world/spawn.cpp:453 |  | TYPED-CHILD SPAWN wrappers (A00 overlay, |
| 0x8013AC34 | LIVE | `Spawn::spawnChildTrigChild` | game/world/spawn.cpp:474 |  |  |
| 0x8013ADBC | LIVE | `beh_box_rearm_sub` | game/ai/beh_box_rearm_sub.cpp:46 |  |  |
| 0x8013AEF0 | LIVE | `beh_a06_spawn_follow_obj_8013AEF0` | game/ai/beh_a06_script_fades.cpp:191 |  | ── FUN_8013AEF0 — spawn a follow-obj and hook it ─────────────────────… |
| 0x8013AFD8 | LIVE | `beh_a06_sound_cmd_wait_8013AFD8` | game/ai/beh_a06_script_fades.cpp:220 | 0x800708B4 | ── FUN_8013AFD8 — kick a sound-command sequence and wait for scratchpa… |
| 0x8013B074 | LIVE | `beh_a06_spawn_subobj_8013B074` | game/ai/beh_a06_script_fades.cpp:257 | 0x8006CBA8 | ── FUN_8013B074 — spawn a subobj + set field/anim params ─────────────… |
| 0x8013B178 | LIVE | `beh_a06_fade_ramp_8013B178` | game/ai/beh_a06_script_fades.cpp:287 |  |  |
| 0x8013B274 | LIVE | `beh_a06_music_cue_8013B274` | game/ai/beh_a06_script_fades.cpp:325 |  |  |
| 0x8013B29C | LIVE | `beh_a06_timer_gate_8013B29C` | game/ai/beh_a06_script_fades.cpp:337 |  | ── FUN_8013B29C — 2-state (init + counted gate) primitive ────────────… |
| 0x8013B2E4 | LIVE | `beh_flagbit_timer_machine` | game/ai/beh_flagbit_timer_machine.cpp:60 |  |  |
| 0x8013B70C | LIVE | `drawInit` | game/ai/beh_seaside_prox_substate.cpp:181 | 0x8013B534 | ======================================================================… |
| 0x8013B868 | LIVE | `subA` | game/ai/beh_seaside_prox_substate.cpp:234 | 0x8006CBA8 0x8006E1C0 0x8006E1E4 | ======================================================================… |
| 0x8013BAB0 | LIVE | `subB` | game/ai/beh_seaside_prox_substate.cpp:314 | 0x8004766C 0x80048750 | ======================================================================… |
| 0x8013BCC8 | LIVE | `subC` | game/ai/beh_seaside_prox_substate.cpp:377 | 0x80027144 0x8003116C 0x8006E1C0 0x8006E1E4 0x8009A450 | ======================================================================… |
| 0x8013C0BC | LIVE | `modeArm` | game/ai/beh_seaside_prox_substate.cpp:133 | 0x80081218 | ======================================================================… |
| 0x8013C1DC | LIVE | `beh_seaside_prox_substate` | game/ai/beh_seaside_prox_substate.cpp:598 |  | 's own prologue is `addiu sp,sp,-0x20` (disas-verified). modeArm/subC'… |
| 0x8013C3F4 | LIVE | `beh_area_threshold_ptr_swap` | game/ai/beh_area_threshold_ptr_swap.cpp:46 |  |  |
| 0x8013C538 | LIVE | `beh_scatter_record_dither` | game/ai/beh_scatter_record_dither.cpp:54 |  |  |
| 0x8013C9C0 | LIVE | `beh_scatter_ramp_machine` | game/ai/beh_scatter_ramp_machine.cpp:49 |  |  |
| 0x8013CDD4 | LIVE | `Render::propQuadRender` | game/render/prop_quad.cpp:42 |  |  |
| 0x8013CDD4 | LIVE | `WidescreenMarginQuad::emit` | game/render/widescreen_margin_quad.cpp:180 |  |  |
| 0x8013D454 | LIVE | `Render::waterJetSpriteRender` | game/render/fx_sprite.cpp:718 |  | 's SPRITE branch — the water jet's other half. The mesh branch (non-ze… |
| 0x8013DD34 | LIVE | `Render::worldLineDraw` | game/render/fx_line.cpp:222 |  | THE rope leaf: a stroke between two world points, drawn as the project… |
| 0x8013DD48 | ORPHAN | `sub8013DD48` | game/ai/beh_a08_scene_actor.cpp:172 | 0x80072DDC | (objAnim, subId) — allocate a spawner obj and hook its handler. |
| 0x8013E08C | LIVE | `Render::shockwaveRingRender` | game/render/fx_line.cpp:375 |  | the expanding SHOCKWAVE RING. Ported 2026-07-28; it was surfaced by |
| 0x8013E9D8 | LIVE | `Render::ropeAnchorRender` | game/render/fx_line.cpp:467 |  | the HANGING object's rope: from the object it hangs off (node+0x14) to… |
| 0x8013EA64 | LIVE | `Render::ropeChainRender` | game/render/fx_line.cpp:484 |  | the segmented CHAIN: 8 points the node carries, joined end to end. nod… |
| 0x8013ED08 | LIVE | `Render::rigidMeshEffectRender` | game/render/fx_rigid_mesh.cpp:48 |  | one rigid packed-mesh controller, rebuilt from its persistent node sta… |
| 0x8013FB88 | LIVE | `OverlayGroundGt3Gt4::gt3` | game/render/overlay_ground_gt3gt4.cpp:142 |  | ground/scene POLY_GT3 emit. Record = 36 bytes, SAME field layout as th… |
| 0x8013FE58 | LIVE | `OverlayGroundGt3Gt4::gt4` | game/render/overlay_ground_gt3gt4.cpp:279 |  | ground/scene POLY_GT4 emit. Record = 44 bytes: {+0 rgb0(rgb1=rgb0<<4)\|… |
| 0x801401B8 | LIVE | `OverlayGroundGt3Gt4::entityLoop` | game/render/overlay_ground_gt3gt4.cpp:408 |  | the ground-entity render list walker. list=a0: +6 (u8) entry count, +1… |
| 0x8014047C | LIVE | `ActorZonedAttacker::gateCheck` | game/ai/actor_zoned_attacker.cpp:146 |  | ActorZonedAttacker::gateCheck(c) — FUN_8014047c(node) -> bool v0. A ti… |
| 0x80140544 | LIVE | `ActorZonedAttacker::typeInit` | game/ai/actor_zoned_attacker.cpp:188 |  | ActorZonedAttacker::typeInit(c) — FUN_80140544(node). One-shot per-typ… |
| 0x801409C0 | LIVE | `ActorZonedAttacker::pickAttackByRange` | game/ai/actor_zoned_attacker.cpp:261 |  | ActorZonedAttacker::pickAttackByRange(c) — FUN_801409c0(node[, unused … |
| 0x80143A00 | LIVE | `ActorZonedAttacker::defaultSubStateMachine` | game/ai/actor_zoned_attacker.cpp:439 |  | ActorZonedAttacker::defaultSubStateMachine(c) — FUN_80143a00(node). Th… |
| 0x80144928 | LIVE | `ActorZonedAttacker::approachAndFace` | game/ai/actor_zoned_attacker.cpp:318 |  | ActorZonedAttacker::approachAndFace(c) — FUN_80144928(node) -> v0. A s… |
| 0x80144B50 | LIVE | `ActorZonedAttacker::idleTick` | game/ai/actor_zoned_attacker.cpp:1180 |  | ActorZonedAttacker::idleTick(c) — FUN_80144b50(node). The "idle" state… |
| 0x80145230 | LIVE | `beh_id_compare_motion_dispatch` | game/ai/beh_id_compare_motion_dispatch.cpp:68 | 0x800781E0 0x8014047C 0x80140544 0x801409C0 0x80143A00 0x80144928 … |  |
| 0x801458E0 | LIVE | `AttackOrbitSubstate::orbitTargetMotion` | game/ai/attack_orbit_substate.cpp:43 |  | node[3]==0x81 sub-behavior: 6-phase acquire/orbit machine, see header … |
| 0x80145AF0 | LIVE | `AttackOrbitSubstate::aimAtTargetAnchor` | game/ai/attack_orbit_substate.cpp:145 |  | node[3]==0x80 sub-behavior: aim-point recompute + one-shot attack-wind… |
| 0x80145C78 | LIVE | `ActorZonedAttacker::zoneClassify` | game/ai/actor_zoned_attacker.cpp:1484 |  | classifies (u8 at record+0x2A, s16 at record+0x36) into a {0,1,2} zone… |
| 0x80146478 | LIVE | `OverlayGt3Gt4::submitBlock` | game/render/overlay_gt3gt4.cpp:127 | 0x801465EC 0x801467BC |  |
| 0x801465EC | LIVE | `OverlayGt3Gt4::gt3` | game/render/overlay_gt3gt4.cpp:176 |  | POLY_GT3 (gouraud-textured triangle) emit, GTE-driven, guest-writing. |
| 0x801467BC | LIVE | `OverlayGt3Gt4::gt4` | game/render/overlay_gt3gt4.cpp:274 |  | POLY_GT4 (gouraud-textured quad) emit, GTE-driven, guest-writing. |
| 0x80182000 | LIVE | `preload_build_vram` | game/core/asset.cpp:379 | 0x80075448 | cel/sprite VRAM build, synchronous. FUN_800753ac is itself an async CD… |
| 0x8018FBCC | LIVE | `CardMenu::install` | game/ui/card_menu.cpp:69 |  |  |
| 0x801FE00C | LIVE | `Render::classifyScene` | game/render/scene_kind_runtime.cpp:8 |  |  |

## PlatformHle-owned (BIOS / hardware-sync primitives — NOT porting targets)

Owned by a DIFFERENT mechanism than the table above: `PlatformHle` (`external/psxport/runtime/psx/platform_hle.cpp`), wired from the addresses this game states in `GameConfig::hle` (`game/core/game_config.cpp`). No native def exists for these, so the scanner above cannot see them — grepping only that table reports them as unowned. The guest body NEVER runs; installing an override on one is a double-install.

| addr | handler | GameConfig::hle field |
|------|---------|-----------------------|
| 0x80080880 | `scheduler_yield` | `changeThread` |
| 0x80080F6C | `syncComplete` | `drawSync` |
| 0x800834A0 | `gpuTimeoutArm` | `gpuTimeoutArm` |
| 0x800834D4 | `syncComplete` | `gpuTimeoutCheck` |
| 0x80085900 | `frameBoundary` | `vsyncTrap` |
| 0x8008A96C | `cdReadSync` | `cdReadSync` |
| 0x8008B2D8 | `syncComplete` | `cdInitHandshake` |
| 0x8008B4B8 | `syncComplete` | `cdDataSync` |
| 0x8009CAEC | `syncComplete` | `decDctInSync` |
| 0x8009CB80 | `syncComplete` | `decDctOutSync` |

10 PlatformHle-owned address(es).

## Deliberately ABSENT — do NOT port from this map alone (`docs/port-map.md`)

Cross-referenced against 57 `docs/port-map.md` steps; 2 carry an explicit `absent:` field. A step here means the layer's PICTURE was removed by decision (usually PROTOCOL.md's absolute no-tap rule). Its entry function may still be natively OWNED above — the producer exists, it just no longer draws — so an owner row is NOT evidence the layer is present. Read the step's `notes` in the port map before touching any of it.

| port-map step | status | why it is absent | guest addrs | owner files |
|---------------|--------|------------------|-------------|-------------|
| `render-producer-effect-mesh-family` | todo | the effect-mesh PICTURE was deleted 2026-08-04 with the GTE-register taps (commit abf3cf9 removed game/render/fx_mesh.cpp/.h, mesh_emit_tap.cpp, swing_fx.cpp/.h). Those producers re-derived quads host-side from the transform the substrate controller had just composed into GTE CR0-7. Deleting them was CORRECT. Four controller-state replacements are live; sixteen native controller producers remain absent. One of those pictures, the `0x8013D454` water-jet mesh, is visible through the explicit `render-fallback-water-jet-guest-gte` hack; fifteen producer-less pictures still have no route. Do not widen that fallback to get the rest back. | 0x80027768 | — |
| `render-producer-submitquad-classes` | todo | the PICTURE for the two REMAINING caller classes (a00-overlay flame/rope emitter, case-188 particles) was DELETED 2026-08-04, not left unported — its GTE-register tap is banned by PROTOCOL.md (USER, absolute). Rebuild ONLY as a native producer reading each emitter's own world state. | 0x8003B320 | — |
