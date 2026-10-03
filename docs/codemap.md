# Repository codemap

Placement authority for both titles. This map does not report progress, goals, issues or evidence.
Execution order belongs in `docs/migration.md`; capability state in `docs/project-state.md`; guest
address ownership in `docs/code-map.md`.

```text
CMakeLists.txt
├── cmake/tomba2_port.cmake              Tomba! 2 product composition
│   └── game/                            Tomba! 2 native title owners
├── titles/tomba1/cmake/tomba1_port.cmake
│   └── titles/tomba1/game/              Tomba! 1 native title owners
└── external/psxport/                    shared Lightrec executor and PSX platform
```

`external/psxport` is a live symlink to the workspace framework checkout; its layout and ownership
chains are described in `external/psxport/docs/codemap.md`. Nothing under it is title-owned.

---

## `game/` — Tomba! 2

One `Engine` per `Core`, reached through the `TombaCtx` aggregate (`game/core/game_ctx.h`) with the
`eng()`, `rend()`, `fade()`, `rngOf()`, `trigOf()`, `mathOf()`, `mtxOf()`, `inv()`, `saveMenuOf()`
accessors. Guest state lives in `Core::mem_*`; the guest register file is `Core::r[]`.

### `game/core/` — application composition, the frame turn, and the override catalog

| Class | Responsibility |
|---|---|
| `Game` (global; from psxport) | The framework's per-session aggregate: `Pad`, `Cd`, `Fmv`, `DbgServer`, `presentation`, `runtime`, `frameDriver`. |
| `TombaCtx` | The title's opaque per-`Core` subsystem aggregate; created by `createTombaContext`, reached from the framework as `Core::gameCtx`. |
| `TombaRuntime` | The one `GameRuntime`: boot (`bootInit`), the frame driver, the title's REPL commands, renderer capability and temporal policy. |
| `TombaFrameDriver` | The single finite per-frame transaction (`stepFrame`): input, timing, task scheduling, native rendering, exactly one presentation fence. |
| `Engine` | The game's stage driver: field run, frame ticks, task machine, object leaves, state dispatch, `frameUpdate`, `drawOTag`. |
| `FrameCadence` | The one owner of the frame-rate decision, published into the guest's own quota byte. |
| `FrameDiagnostics` | Per-frame title state probes and counters reported after the frame. |
| `AutoDrive` | Title-aware stage/area automation applied at the frame boundary. |
| `LibapiIntr` | libapi interrupt-mask primitives; mirrors the host VBlank count to the guest word at the title frame boundary. |
| `Asset` | LZ decompress, texture-group unpack, CPU→VRAM upload, and the stage preload chain. |
| `Str` | The resident native string leaves, registered image-aware. |
| `tomba::native::declareOverride` / `declareOverlayOverride` / `bindResident` / `activateOverlay` / `activateModeOverlay` / `activateAreaSlotOverlay` / `retireOverlay` / `loadAreaSlotFile` | The one native-override declaration catalog; a declaration is keyed by image identity plus guest address, never by address alone. |
| `TombaConfig` (`game_config.cpp`) | The measured Tomba! 2 compatibility facts (guest addresses and sizes) the frame driver requires. |
| `VerificationCounters` | Title-owned verification tallies surfaced through the diagnostics channel. |
| `tomba::scene::stepCardLoadMachine` (`dev_warp.h`, `task_sm.h`) | Dev warp arming, and the generic task state-machine vocabulary. |

### `game/input/`
`Engine::padEdgeFence` — the port of `FUN_800788AC`, the pad-edge fence that keeps one edge from
re-triggering within a frame.

### `game/ai/` — per-object and per-area behavior bodies

Namespace `tomba::ai` owns the zoned-attacker cluster; the older clusters keep their own feature
namespaces (`tomba::ai::actorbump`, `::actorcontact`, `::ropeswing`, `::swaynode`, `::tiltfollow`,
`::areaseq`, `::objcontact`, `::ropenode`, `::tiltpart`). Every `beh_*` file defines one static
member of `Behaviors` or one guest-ABI leaf; `Behaviors` (`game/ai/behaviors.h`) is the address-keyed
dispatch surface.

| Class | Responsibility |
|---|---|
| `Behaviors` | The per-object / per-area behavior handler surface; one member per handler, one file per member. |
| `tomba::ai::ActorZonedAttacker` | The A00-overlay zoned-attacker sub-behavior cluster (gate, type init, attack pick, approach, sub-state, idle, zone classify). |
| `ActorBump`, `ActorObjectContact`, `ActorMeleeEngage`, `MeleeProximity` | The contact-resolution behaviors: bump, object contact, melee engagement and its proximity test. |
| `AssemblyNode`, `AssemblyChild`, `AssemblyCompanion`, `AssemblyRider`, `RiddenAssembly` | Multi-part actor assembly: node/child structure, companions, and riders. |
| `AttackOrbitSubstate`, `ReleaseTriggerMotion`, `SubstateEdgeLeaves`, `PlacedPropSm` | Orbital attack, release-trigger motion, sub-state edge transitions, and placed-prop state machines. |
| `RopeSwing`, `SwaySchedule`, `TiltFollower` | Rope nodes and swing, sway schedules, and tilt followers. |
| `zoned_attacker_abi.h` | The zoned attacker's shared guest ABI vocabulary: register slots, leaf and global addresses, the two leaf-call shorthands, and the guest-stack frame contracts. |

### `game/object/` — guest object structure and the per-object dispatchers

| Class | Responsibility |
|---|---|
| `Actor` | The named-field lens over a guest object node. |
| `BehaviorDispatch` | The per-object behavior-handler dispatcher. |
| `ObjectList` | The per-frame entity-list walkers. |
| `Animation` | The per-object animation VM. |
| `ScriptVm` (`script_vm.h`) | The per-object script VM. |
| `Array8Dispatch` | The per-node array-8 handler dispatch. |
| `ActorReward` | The item/reward actor behavior. |
| `CubeTextLedger` | The cube-text banner's live text ledger. |

### `game/player/` — Tomba's actor and its collision

| Class | Responsibility |
|---|---|
| `tomba::player::ActorTomba` (`actor_tomba.{h,cpp}`) | The Tomba actor: a named-field view over the master guest block plus its owned per-frame logic — the G-block driver, the growth/motion/transition cascade, and the auxiliary-list walks that decide which item Tomba is standing on. |
| `tomba::player::ActorInteraction` (`actor_interaction.{h,cpp}`) | What touching an item means: the proximity/type-4/sub-hitbox checks that pick a touched item, and one leaf per guest item kind (interact mode, type 8, type 7). `ActorTomba`'s walks dispatch into it; it reads the G block through its owner and calls back through `ActorTomba::growthYSnap`. |
| `tomba_state.h` | The shared Tomba vocabulary both owners need: the G-block field lens (`TombaState`) and the guest addresses every Tomba subsystem keys off. |
| `Collision` | The collision-grid subsystem. |
| `tomba::targeting::ActorTargeting` | Whether the actor may acquire a target: near enough and in the right direction. |
| `Hitbox`, `GridOffset` (`hitbox.h`, `grid_offset.h`) | The per-object 2D box/hitbox-corner builder, and the collision-grid offset resolution. |

### `game/world/` — field object population and geometry

| Class | Responsibility |
|---|---|
| `Spawn` | Entity spawn and despawn. |
| `Pool` | Object-pool and per-area control-block initialization. |
| `AreaSlots` | The area-slot state machine. |
| `ObjectTable` | The 40-slot fixed object table dispatcher. |
| `Placement` | Field object placement. |
| `Entity` | The per-object entity state machine. |
| `CollisionResolve` | Actor-vs-object cylinder collision resolve. |
| `GraphicsBind` | The object render-bind subsystem. |

### `game/scene/` — stage machines and loading

| Class | Responsibility |
|---|---|
| `Demo` | The DEMO / front-end menu stage state machine. |
| `Sop` | The SOP (intro cutscene) field stage machine. |
| `StartBinStage` | Task-0's disc file tables and the boot preloads for both execution models. |
| `ScriptInterp`, `tomba::scene::ScriptObject`, `ScriptOpcode` vocabulary (`script_opcode.h`, `script_globals.h`) | The cutscene script interpreter, its typed lens over the driven object, and its vocabulary. |
| `FieldTransition`, `BgSceneTransitionSm`, `SceneTransition`, `TransitionState3` | The four scene-transition machines: the sm[0x4a]==5 area fade, the SOP background fade, the scene/sub-scene swap, and the mid-transition entity walk. |
| `SceneEvents` | The scene-event arm subsystem. |
| `ParallaxBg`, `parallax_scroll.h` | The SOP field-mode parallax background and its two scroll-domain policies. |
| `ModeStateArm` | The mode-state arm primitive pair. |
| `tomba::stage::loadOverlay` (`level_load.h`) | Load START/DEMO/GAME into the shared stage slot, retire replaced generations, publish the loaded image. |
| `tomba::scene::stepCardLoadMachine` (`card_load_machine.h`) | The memory-card LOAD/SAVE page sub-machine and CRD image residency in the AREA slot. |
| `startup.cpp` | Engine startup/init, reimplemented from the game's own main entry point. |

### `game/audio/` — the sound driver's native leaves

Namespace `tomba::audio`. `Sequencer` is split by responsibility across
`sequencer.cpp` (tick + scheduler), `sequencer_channel_flags.cpp`, `sequencer_pitch_envelope.cpp`,
`sequencer_voice_write.cpp`, `sequencer_tone_records.cpp`, `sequencer_voice_alloc.cpp` and
`sequencer_voice_state.cpp`; each group declares its own overrides through a `declare*Overrides()`
entry point declared in `sequencer.h` and called from `Sequencer::registerOverrides`.

| Class | Responsibility |
|---|---|
| `Sequencer` | libsnd's per-VBlank sequencer: the tick wrapper, the sequence/channel scheduler, and the channel leaves it routes to. |
| `tomba::audio::record::ChannelRecord` | The typed lens over one per-(sequence, channel) record. |
| `tomba::audio::libsnd::*` (`libsnd_globals.h`) | The sound driver's global cluster, one named offset per word. |
| `Sfx` | The sound-FX trigger dispatcher. |
| `AudioDispatch` | The field audio dispatch/settle cluster. |
| `MusicCoord` | The per-field music coordination tick. |
| `MusicList` | The Sound Test catalogue and the in-game area BGM driver. |
| `NativeMusic` | The real-time SEP/VAB music player mixed into the SPU sink. |

### `game/camera/`

| Class | Responsibility |
|---|---|
| `tomba::camera::CutsceneCamera` (`cutscene_camera.{h,cpp}`) | The cutscene/free camera: follow modes, head/pitch/heading solve, distance and shake, and the per-mode motion the look builder's points feed. |
| `tomba::camera::LookAngleBuilder` (`camera_look_builder.{h,cpp}`) | The look-point/heading construction the camera solve is built from — yaw/distribution accumulation, the four point builders, the table joins, and the rotation/distance solve. A collaborator of `CutsceneCamera`, not part of it. |
| `camera_state.h` | The camera guest-memory lens (`CameraState`): the master/scratch blocks and the camera object slot, shared by the camera owner and its look builder. |
| `camera_guest_math.h` | The camera's shared guest math vocabulary — the fixed-point helpers and sign conventions every camera solve agrees on. |
| `tomba::camera::ModeDescriptor` / `RenderModePrologue` (`camera_mode.h`) | The camera driver's mode table, in one place. |

### `game/cd/` — libcd's guest-visible CD surface

| Class | Responsibility |
|---|---|
| `LibcdDirCache` | Writes libcd's guest directory/file cache from host ISO9660 sectors (`CdNewMedia`, `CdCacheFile`). |
| `LibcdNative` | The native wrapper around the substrate's libcd chain for the file-table build. |

### `game/items/`

| Class | Responsibility |
|---|---|
| `Inventory` | The item / inventory-collection subsystem. |

### `game/math/`

| Class | Responsibility |
|---|---|
| `Math` | The hot per-frame GTE-transform cluster: rotation-matrix compose and apply. |
| `GteTransform3` | The three-axis GTE transform. |
| `Mtx` | libgte matrix helpers. |
| `Trig` | libgte trig helpers (rsin, rcos, ratan2, angle compare). |
| `Rng` | The LFSR pseudo-random generator (guest `FUN_8009A450`). |
| `Bit` (`mathlib.h`) | The bitmap flag bit-test primitives. |

### `game/ui/` — native UI producers

| Class | Responsibility |
|---|---|
| `Font` | Engine font/text initialization. |
| `Panel` | The native 2D UI panel family. |
| `UiSprite`, `UiGroupCapture` (`ui_group_capture.h`) | UI sprite submission and the templated UI group capture. |
| `PauseMenu` | The in-game pause / item menu producer. |
| `StartPage`, `OptionsPage`, `SavePrompt`, `SaveMenu`, `CardMenu` | The authored front-end and in-field pages. |
| `DialogTextStream` | The dialog typewriter stream. |
| `menu.h`, `bav_loader.cpp` | The in-game options menu subsystem and the BAV animation loader. |

### `game/render/` — the native picture

| Class | Responsibility |
|---|---|
| `Render` | The render subsystem umbrella: the frame's producers and their submit order. |
| `NativeScenePass` (`render_native.h`) | The native render pass itself. |
| `RenderScene`, `SceneCamera`, `SceneObject` (`scene_data.h`) | The frame's intermediate representation. |
| `NodeXform` | The scene-node world-transform builder. |
| `ObjModelView` | The shared model-view setup leaf every effect-mesh draw runs first. |
| `Cull` | Visibility culling and LOD. |
| `tomba2::horizontal_cull::Visibility` (`horizontal_visibility_cull.h`) | The title's recovered horizontal visibility cull, owned once. |
| `Lighting` | The per-area light registry. |
| `MeshQuads` | The host-side builders for packed object rotation matrices. |
| `QuadRtptSubmit` | The two shared GTE quad-submit leaves. |
| `GuestQueueDispatch` (`queue_dispatch.h`) | The guest's own dispatch for the three cull render queues. |
| `GuestRngMirror` | A read-only stand-in for the guest PRNG. |
| `LibgpuDrawEnv` | The DRAWENV → DR_ENV compiler. |
| `ScreenFade` | The screen-fade subsystem. |
| Effect producers (`fx_sprite*`, `fx_*`, `prop_quad.h`, `tile_grid_layer.h`, `objlist_walk.cpp`, `subpart_walk*.cpp`, `perobj_*.cpp`, `overlay_gt3gt4.*`, `overlay_ground_gt3gt4.*`, `native_terrain.cpp`, `minimap.cpp`, `field_hud.cpp`) | One native producer per guest draw family; `fx_sprite.h` holds the shared contract, `fxpublish` the scratchpad handoff, `fxswarm`/`fxanchored` the family members. |
| UI producers (`ui_ft4_tap.h`, `ui_group_args.h`, `cube_text_banner.h`, `score_popup.h`, `hud_gauge_emitter.h`) | The shared templated UI group leaves and the announcement producers. |
| Interpolation (`effect_lerp.h`, `fps60_worldpass.cpp`) | `EffectLerp` — the actor-transform interpolation tier; `Fps60::frame_commit` — the 60 fps world-pass policy. |
| Widescreen (`wide_window.h`, `margin_render.h`, `title_wide_composition.cpp`, `widescreen_margin_quad.h`, `wide_re_*.cpp`, `horizontal_visibility_cull.h`, `page_gradient.h`, `page_backdrop.h`) | The horizontal window, the margin renderer, the title-wide composition, the margin-quad producer, the retained-W field coverage, and the authored page gradients and their widened margins. |
| Policy seams (`scene_kind.h`, `area21_sky_gradient_policy.h`, `object_highlight_policy.h`, `ot_key_ord_policy.h`, `guest_face_gate.h`) | The pure decisions the producers share, each with its own unit test. |

---

## Who owns it

Each chain names the class and method at every hop.

### The frame turn

```text
psxport FrameLoopShell::step
  → binds the per-Core process globals (gte/projprim/spu/mdec/xa) once per session
  → tomba::TombaFrameDriver::stepFrame            game/core/frame_driver.cpp
      → Engine::cadence().beginLogicFrame         game/core/frame_cadence.cpp
      → Game::hle.deliverEvent                    (psxport)
      → Pad::serviceFrame                         (psxport; writes the guest pad packet)
      → Engine::frameUpdate                       game/core/engine_frame_ticks.cpp
          → the guest task machine, native object leaves, native scene state
      → Game::presentation.commit                 (psxport; exactly one fence)
      → TombaFrameDriver's submitFrame            guest DrawSync + PutDrawEnv + Engine::drawOTag
      → Engine::cadence().endLogicFrame
      → FrameDiagnostics::afterFrame, AutoDrive::afterFrame
  → gpu_present_frame_capture                     (psxport; once, after whichever presenter ran)
  → asserts the fence advanced exactly once
```

**While a movie or a blocking load runs.** Tomba's movies block: the boot movies
(`native_boot.cpp`, psxport) and `OP.STR` (`Demo`, `game/scene/demo.cpp`) call
`Fmv::play`, which owns the whole turn until the movie ends. Nothing else services the pad meanwhile,
so `Fmv::playToEnd` calls `Pad::pollHostInput` before every `Fmv::step`; that is the only way the
player's Start reaches the skip check, and the control channel does not answer until the movie ends.
A loading call runs inside the guest's own `Engine::frameUpdate`, inside the same `stepFrame`; there
is no separate loading turn and no loading-only screen.

### Host input → `Pad` → the guest pad buffer

```text
psx::input::HostInput                        runtime/psx/host_input.*  — the SDL drain, key state,
                                                                    gamepads, and the keyboard gate
  → Pad::pollHostInput                       runtime/psx/pad_input.cpp — the ONE host pump; also the
                                                                    P / '.' debug keys. It asks
                                                                    gpu_vk_windowed()
                                                                    (runtime/psx/gpu_vk.h) whether it
                                                                    is windowed or headless first.
      → Pad::serviceFrame                    resolves host / forced / REPL / replay into Pad::buttons,
                                             samples edges, records or replays, and writes the guest
                                             packet through Pad::fillBuffer
          → called once per logic frame from TombaFrameDriver::stepFrame
          → movie skip: Fmv::step reads Start held in Pad::buttons (psxport)
          → the debug channel's pause/step comes from Pad::pollHostInput
```

The title never reads host input itself: `game/input/pad_edge_fence.cpp` consumes the already-resolved
mask. Engine per-VBlank pad edges are `Engine::frameUpdate`'s work.

### Guest draw → presentation

```text
Engine::drawOTag                             game/core/engine.cpp
  → Render's producers                       game/render/*.cpp — native scene/UI passes, each reading
                                               the owning game state
      → GuestQueueDispatch                   the guest's three cull render queues
      → submit.cpp / projprim / GTE           transform, projection, primitive submission
  → RenderQueue                              flushes to the psxport render queue
  → Game::presentation.commit                (psxport)
      → the real field presents here
      → Fps60::frame_commit / EffectLerp      an interpolated in-between frame presents through the
                                               same renderer, using the prior and current state
      → widescreen                            the projection/window owners (wide_window, cull,
                                               title_wide_composition, margin_render) decide what
                                               the widened canvas shows; nothing is ever stretched
  → gpu_present_frame_capture                 once, at the tail of the turn
```

### CD and streaming

```text
Game::cd                                    runtime/psx/cd.* — the CD subsystem
  → CdcNative / CdcCommandPhase / CdPosition (psxport) — the drive state machine
  → StockReadLanding, XA streaming           (psxport)
  → title side: LibcdDirCache                game/cd/libcd_dir_cache.cpp — writes the guest's own
                                               directory cache from host ISO9660 sectors
             LibcdNative                     game/cd/libcd_native.cpp — the substrate libcd chain
             tomba::stage::loadOverlay       game/scene/level_load.cpp — START/DEMO/GAME
             tomba::native::activateModeOverlay / activateAreaSlotOverlay
                                               — publish a loaded image and bind its overrides
  → Game::cd.audioTrace                      the per-frame audio/CD phase trace the driver logs
```

### Audio

```text
Engine::musicCoord.tick                     game/audio/music_coord.cpp — coordinates the field's
                                               audio requests
  → Sfx                                       game/audio/sfx.cpp — the sound-FX trigger dispatcher
  → AudioDispatch                             game/audio/audio_dispatch.cpp — the field audio
                                               dispatch/settle cluster
  → MusicList / NativeMusic                   the Sound Test catalogue and the real-time SEP/VAB synth
  → Sequencer::frameTick                      game/audio/sequencer.cpp — libsnd's per-VBlank tick
                                               wrapper, then Sequencer::seqChannelDispatch, which
                                               routes each channel's flags bit to the matching leaf
  → Spu                                       (psxport) — the actual SPU voice state
```

### The debug / control channel

```text
lucent::http::Server                         (psxport) — the loopback listener, always open
  → psx::dbg::DbgServer                      runtime/psx/dbg_server.* — the command surface
      → framework commands                   (psxport) — memory, input, screenshot, render toggles
      → GameRuntime::replCommand → tomba::TombaRuntime::replCommand
          → game/core/repl_commands.cpp       the title's commands, which reach Tomba! 2 classes and
                                               guest layouts without the framework naming either
      → Debug warp / area selection          game/core/dev_warp.* and game/core/dev_areas.cpp — a
                                               request is armed and the frame driver applies it at a
                                               frame boundary through the game's own transition and
                                               load owners
```

---

## `titles/tomba1/` — Tomba! 1

Everything lives under `titles/tomba1/`; it reuses only the generic psxport executor and services and
never a Tomba! 2 address, overlay assumption, owner or renderer policy.

| Path | Responsibility |
|---|---|
| `game/app/` | The process-lifetime composition only. |
| `game/core/` | Startup, the frame transaction, CD/DMA, and the deliberately native behavior, declared by complete image identity. |
| `game/render/` | Executable-derived projection, visibility, edge coverage and 2D layout for widescreen. No native renderer, producers, depth, interpolation or 60 fps. |
| `tests/` | Tomba! 1's unit tests, including the 35-field CRT0 boundary and the widescreen projection. |

---

## Placement rules

| Concern | Owner |
|---|---|
| Lightrec decode/emission, CPU synchronization, bounded exits, image generations, invalidation, generic override/original-call dispatch, host input, CD/GPU/SPU/MDEC services, the frame loop shell, movies, presentation | shared `external/psxport` |
| A Tomba! 2 guest address, gameplay rule, object, scene, native producer, or override selection | the `game/` subsystem that owns that responsibility |
| Tomba! 2 per-frame input, timing, scheduler, render order, or the presentation boundary | `game/core/frame_driver.cpp` |
| Tomba! 2 libetc VBlank guest address and callback write order | `game/core/libapi_intr.*`, sequenced by the frame driver |
| Tomba! 2 stage-aware automation, guest-layout probes, title-specific REPL inspection | `game/core/auto_drive.*`, `game/core/frame_diagnostics.*`, `game/core/repl_commands.cpp` |
| A Tomba! 1 guest address, runtime, gameplay rule, projection or layout policy | the matching `titles/tomba1/game/` subsystem |
| Offline guest-code emission, generated registries, interpreter-first gameplay selection | nowhere; these have no target owner |
| Epic intent | `docs/project-goals.md` |
| Capability coverage | `docs/project-state.md` |
| Atomic work | `docs/issues/` |