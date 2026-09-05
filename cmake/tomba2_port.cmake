# cmake/tomba2_port.cmake — compose the native Tomba! 2 owners around psxport's Lightrec runtime.

option(PSXPORT_BUILD_PORT "Build the Tomba!2 native port binary (tomba2_port)" ON)

# The framework static library (psxport) + its option(PSXPORT_BUILD_SMOKE) + the standalone smoke.
# Always included so `psxport` / `psxport_smoke` are buildable even when the game target is off.
include(${PSXPORT_DIR}/cmake/psxport.cmake)

if(NOT PSXPORT_BUILD_PORT)
  return()
endif()

if(NOT EXISTS "${PSXPORT_DIR}/runtime/cpu/dynarec_capabilities.h" OR
   NOT EXISTS "${PSXPORT_DIR}/runtime/cpu/native_dispatch.h")
  message(FATAL_ERROR
    "Tomba! 2's offline guest-source product was removed by the break-first migration, but "
    "PSXPORT_DIR=${PSXPORT_DIR} does not expose the required Lightrec runtime address API "
    "(runtime/cpu/dynarec_capabilities.h and runtime/cpu/native_dispatch.h).")
endif()

execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_dynarec_boundary.py"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  RESULT_VARIABLE TOMBA2_DYNAREC_BOUNDARY_RESULT
  OUTPUT_VARIABLE TOMBA2_DYNAREC_BOUNDARY_OUTPUT
  ERROR_VARIABLE TOMBA2_DYNAREC_BOUNDARY_ERROR)
if(NOT TOMBA2_DYNAREC_BOUNDARY_RESULT EQUAL 0)
  message(FATAL_ERROR
    "Tomba! 2's offline guest-source product was removed, but its native override graph still names "
    "retired bindings. Finish the image-aware Lightrec registration boundary before "
    "building the product.\n${TOMBA2_DYNAREC_BOUNDARY_OUTPUT}${TOMBA2_DYNAREC_BOUNDARY_ERROR}")
endif()

# ---- game source list (game/* only — the framework moved to cmake/psxport.cmake) --------------
set(GAME_SRC
  game/game_tomba2.cpp
  game/cd/libcd_native.cpp
  game/core/asset.cpp
  game/core/auto_drive.cpp
  game/core/dev_warp.cpp
  game/core/frame_diagnostics.cpp
  game/core/frame_driver.cpp
  game/core/libapi_intr.cpp
  game/core/native_override_catalog.cpp
  game/core/game_config.cpp
  game/core/game_ctx.cpp
  game/core/game_hooks.cpp
  game/core/tomba_runtime.cpp
  game/core/main.cpp                # process entry point (P1.7c: main() is game-side)
  game/render/fps60_worldpass.cpp   # TRANSITIONAL fps60 world-pass hook body (P1.7c)
  game/core/dev_areas.cpp
  game/core/repl_commands.cpp
  game/core/register_overrides.cpp
  game/core/str.cpp
  game/math/mathlib.cpp
  game/math/rng.cpp
  game/math/mtx.cpp
  game/math/trig.cpp
  game/render/cube_text_banner.cpp
  game/render/cull.cpp
  game/player/collision.cpp
  game/player/actor_targeting.cpp       # FUN_8001FAE0 — acquire a target: reach, band, arc
    game/player/interact_scan.cpp
  game/player/hitbox.cpp
  game/player/grid_offset.cpp
  game/world/spawn.cpp
  game/scene/scene_events.cpp
  game/scene/script_interp.cpp
  game/audio/sfx.cpp
  game/audio/audio_dispatch.cpp
  game/audio/sequencer.cpp
  game/world/area_slots.cpp
  game/scene/mode_state_arm.cpp
  game/world/placement.cpp
  game/world/graphics_bind.cpp
  game/world/pool.cpp
  game/world/entity.cpp
  game/world/collision_resolve.cpp
  game/render/render_native.cpp
  game/render/scene_build.cpp
  game/render/mesh_draw.cpp
  game/object/actor_sm_24448.cpp
  game/object/actor_sm_reward.cpp
  game/object/cube_text_ledger.cpp
  game/ai/beh_scene_ui_trigger.cpp
  game/ai/beh_typed_init_scene_trigger.cpp
  game/ai/beh_pickup_collect_trigger.cpp
  game/ai/beh_substate_edge_orchestrator.cpp
  game/ai/substate_edge_native.cpp
  game/ai/assembly_companion.cpp         # FUN_80138A64 idle tick (camera hold + re-arm) + FUN_801389C8 rig pose
  game/ai/assembly_rider.cpp             # FUN_80118B10 rider perched on a seaside pump's arm-end (ride/hop/fling)
  game/ai/tilt_follower.cpp             # FUN_80125FE0 — pitch at half a sub-part's tilt
  game/ai/sway_schedule.cpp             # FUN_8012D27C — rocking rate winds down over the area's event sequence
  game/ai/rope_swing.cpp                # FUN_801281B8 — hanging rope: spring swing + per-segment bend
  game/ai/actor_object_contact.cpp      # FUN_8010E258 — actor-vs-object hit / proximity contact
  game/ai/actor_bump.cpp                # FUN_8010EA80 — actor bump: interact / push apart / recoil
  game/ai/placed_prop_sm.cpp
  game/ai/beh_jumptable_release_trigger.cpp
  game/ai/release_trigger_motion.cpp
  game/ai/beh_typed_table_seed_gate.cpp
  game/ai/beh_typed_jumptable_pair.cpp
  game/ai/beh_cull_substate_orchestrator.cpp
  game/ai/beh_id_compare_motion_dispatch.cpp
  game/ai/actor_zoned_attacker.cpp
  game/ai/attack_orbit_substate.cpp
  game/ai/actor_melee_engage.cpp
  game/ai/beh_actor_tomba_proximity_combat.cpp
  game/ai/melee_proximity.cpp
  game/ai/beh_jumptable_flag_gate.cpp
  game/ai/beh_cull_tick_render.cpp
  game/ai/beh_sibling_angle_track.cpp
  game/ai/beh_visibility_gate_dispatch.cpp
  game/ai/beh_record_list_scanner.cpp
  game/ai/beh_area_event_dispatch.cpp
  game/ai/beh_pad_child_linker.cpp
  game/ai/beh_scatter_record_dither.cpp
  game/ai/beh_area_threshold_ptr_swap.cpp
  game/ai/beh_scatter_ramp_machine.cpp
  game/ai/beh_pure_inner_dispatch.cpp
  game/ai/beh_anim_trigger_gates.cpp
  game/ai/beh_box_seed_phase_gate.cpp
  game/ai/beh_typed_anim_spawn.cpp
  game/ai/beh_id_routed_dispatch.cpp
  game/ai/beh_pure_substate_dispatch.cpp
  game/ai/beh_linked_advance_branch.cpp
  game/ai/beh_typed_init_exit_poker.cpp
  game/ai/beh_child_trig_motion.cpp
  game/ai/beh_prng_velocity_machine.cpp
  game/ai/beh_quad_record_table_seed.cpp
  game/ai/beh_flagbit_timer_machine.cpp
  game/ai/beh_two_child_steer.cpp
  game/ai/beh_single_child_cull.cpp
  game/ai/beh_twin_record_steer.cpp
  game/ai/beh_multi_record_phase_machine.cpp
  game/ai/beh_sine_motion_sfx.cpp
  game/ai/beh_box_rearm_sub.cpp
  game/ai/beh_node3_router.cpp
  game/ai/beh_actor_move_sm.cpp
  game/ai/beh_variant_actor_sm.cpp
  game/ai/beh_lift_platform.cpp
  game/ai/beh_event_record_machine.cpp
  game/ai/beh_typed_variant_router.cpp
  game/ai/beh_camera_target_follow.cpp
  game/ai/beh_cube_text_spawn.cpp
  game/ai/beh_area_transition_machine.cpp
  game/ai/beh_rand_phase_cull.cpp
  game/ai/beh_pos_history_trail.cpp
  game/ai/beh_variant_overlay_lifecycle.cpp
  game/ai/beh_a06_multi_actor.cpp
  game/ai/beh_a06_scripted_actor.cpp
  game/ai/beh_a06_script_fades.cpp
  game/ai/beh_a08_scene_actor.cpp
  game/ai/beh_toy_spawn_family.cpp
  game/ai/beh_sop_intro_pilot.cpp
  game/ai/beh_sop_intro_lifted.cpp
  game/ai/beh_sop_intro_narration.cpp
  game/ai/sop_overlay_shadow.cpp
  game/ai/sop_intro_events.cpp
  game/ai/beh_seaside_prox_substate.cpp
  game/ai/area_seaside_perframe.cpp
  game/ai/beh_substate_edge_leaves.cpp
  game/player/actor_tomba.cpp
  game/scene/bg_scene_transition_sm.cpp
  game/scene/parallax_bg.cpp
  game/scene/scene_transition.cpp
  game/scene/transition_state3.cpp
  game/object/object_list.cpp
  game/object/array8_dispatch.cpp
  game/world/object_table.cpp
  game/object/script_vm.cpp
  game/object/animation.cpp
  game/input/pad_edge_fence.cpp
  game/ui/menu.cpp
  game/ui/ui_sprite_compose.cpp
  game/ui/ui_sprite.cpp
  game/ui/loading_text.cpp
  game/ui/panel_fill.cpp
  game/ui/dialog_backdrop.cpp
  game/ui/dialog_text_stream.cpp
  game/items/inventory.cpp
  game/render/lighting.cpp
  game/ui/bav_loader.cpp
  game/ui/save_menu.cpp
  game/audio/music_coord.cpp
  game/scene/startup.cpp
  game/ui/font.cpp
  game/ui/panel.cpp
  game/ui/pause_menu.cpp
  game/ui/start_page.cpp
  game/ui/card_menu.cpp
  game/ui/options_page.cpp
  game/ui/ui_group_capture.cpp
  game/scene/level_load.cpp
  game/object/behavior_dispatch.cpp
  game/render/submit.cpp
  game/render/node_xform.cpp
  game/render/projection.cpp
  game/render/render_frame.cpp
  game/render/cine_bars.cpp
  game/render/narration_swirl.cpp
  game/render/render_walk.cpp
  game/render/title_wide_composition.cpp
  game/render/scene_kind.cpp
  game/render/scene_kind_runtime.cpp
  game/render/render_hut_interior.cpp   # pc_render producer: hut/door authored sub-scene (objects-only)
  game/render/card_browser.cpp          # pc_render producer: DEMO/title Load-Game card browser (s48==4)
  game/render/render_options.cpp        # pc_render producer: DEMO/title options page (s48==6)
  game/render/render_attract.cpp        # pc_render producer: DEMO/title attract 3D field (s48==7)
  game/core/engine.cpp
  game/scene/sop.cpp
  game/scene/demo.cpp
  game/camera/cutscene_camera.cpp
  game/math/gte_math.cpp
  game/math/wide_re_gte_transform3.cpp
  game/render/wide_re_libgpu_leaves.cpp
  game/render/wide_re_gpu_dma_queue.cpp
  game/render/wide_re_gpu_loadimage_streamer.cpp
  game/render/wide_re_gpu_putdrawenv.cpp
  game/render/libgpu_draw_env.cpp        # libgpu SetDrawEnv (0x80081FB0) — DRAWENV -> DR_ENV packet
  game/render/native_terrain.cpp
  game/render/screen_fade.cpp
  game/render/margin_render.cpp
  game/render/quad_rtpt_submit.cpp
  game/audio/native_audio.c
  game/audio/native_music.cpp
  game/audio/music_list.cpp
  game/render/overlay_gt3gt4.cpp
  game/render/overlay_ground_gt3gt4.cpp
  game/render/tile_grid_layer.cpp
  game/render/widescreen_margin_quad.cpp
  game/render/obj_model_view.cpp
  game/render/hud_gauge_emitter.cpp
  game/render/fx_sprite.cpp
  game/render/fx_sprite_swarm.cpp        # FUN_800281EC — per-particle member of the sprite family
  game/render/fx_sprite_anchored.cpp     # FUN_80027CB4 — single-anchor uniform-scale member
  game/render/fx_trail.cpp
  game/render/fx_dotfield.cpp
  game/render/fx_backdrop_plane.cpp
  game/render/fx_motes.cpp
  game/render/guest_rng_mirror.cpp
  game/render/fx_vortex.cpp
  game/render/fx_beam.cpp
  game/render/fx_rope_strip.cpp
  game/render/fx_line.cpp
  game/render/fx_ring.cpp
  game/render/fx_dust.cpp
  game/render/fx_impact.cpp
  game/render/object_highlight.cpp
  game/render/area21_sky_gradient.cpp
  game/render/backdrop.cpp
  game/render/fx_plume.cpp
  game/render/fx_rigid_mesh.cpp
  game/render/fx_swing.cpp
  game/render/prop_quad.cpp
  game/render/guest_gte_water_jet.cpp
  game/render/mesh_quads.cpp
  game/render/effect_lerp.cpp
  game/render/field_hud.cpp
  game/render/minimap.cpp
  game/render/ui_group_args.cpp
  game/render/ui_ft4_tap.cpp
  game/render/score_popup.cpp
  game/render/perobj_dispatch.cpp
  game/render/perobj_billboard.cpp
  game/render/subpart_walk_shared.cpp
  game/render/subpart_walk.cpp
  game/render/compose_tint_gate.cpp
  game/render/effect_mod.cpp
  game/render/text_label.cpp
  game/render/render_walk_dispatch.cpp
  game/render/overlay_type_dispatch.cpp
  game/render/objlist_walk.cpp
  game/render/queue_dispatch.cpp)

add_executable(tomba2_port ${GAME_SRC})
# The framework's shader header is generated by the psxport library's custom target; the game exe
# (via gpu_vk.cpp in libpsxport) transitively needs it present before its own compile ordering.
add_dependencies(tomba2_port gen_gpu_shaders)

# C++17 for this target (engine), matching the framework library.
set_target_properties(tomba2_port PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
  ENABLE_EXPORTS ON                                   # -rdynamic: watchdog backtrace symbol names
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# GAME include dirs. The framework include dirs (RT, generated, vendored backends, SDL/freetype) are
# inherited PUBLICly from the psxport link below — only the game/* subfolders are added here.
target_include_directories(tomba2_port PRIVATE
  game  game/ai  game/audio  game/camera  game/cd  game/core  game/input  game/items  game/math  game/object  game/player  game/render  game/scene  game/ui
  game/world)

target_compile_options(tomba2_port PRIVATE -w -O2 -g
  ${SDL3_CFLAGS_OTHER} ${FREETYPE_CFLAGS_OTHER})

# The framework library carries all system/vendored link deps + compile defs as PUBLIC, so linking it
# is all the game exe needs.
target_link_libraries(tomba2_port PRIVATE psxport)

if(BUILD_TESTING)
  foreach(TOMBA2_HELP_ARGUMENT IN ITEMS -h --help)
    string(REPLACE "-" "" TOMBA2_HELP_SUFFIX "${TOMBA2_HELP_ARGUMENT}")
    add_test(
      NAME "tomba2_direct_help_${TOMBA2_HELP_SUFFIX}"
      COMMAND "$<TARGET_FILE:tomba2_port>" "${TOMBA2_HELP_ARGUMENT}")
    set_tests_properties(
      "tomba2_direct_help_${TOMBA2_HELP_SUFFIX}"
      PROPERTIES PASS_REGULAR_EXPRESSION "Usage: tomba2_port")
  endforeach()
endif()
