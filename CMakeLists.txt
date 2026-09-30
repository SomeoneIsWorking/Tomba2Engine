# Tomba PC ports, built on the shared psxport Lightrec runtime. The repository-level game/** tree
# belongs only to Tomba! 2. Tomba! 1 is a separate engine under titles/tomba1/** and may consume
# psxport, but it must never compile or include Tomba! 2 game code.
cmake_minimum_required(VERSION 3.21)
project(tomba2engine CXX C)

include(CTest)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# PSXPORT_DIR — WHERE THE FRAMEWORK SOURCE COMES FROM. Defaults to external/psxport, which is NOT a
# git submodule any more: it is a symlink to the workspace's shared framework clone (local — so a
# framework edit is live in every port at once), or a private clone at psxport.pin (fresh machine, CI,
# or anyone cloning this repo alone). tools/psxport_fetch.py --auto establishes whichever applies, and
# run.sh calls it. The PATH is unchanged, so every reference to external/psxport/... still works.
#
# Point it at the workspace's framework DEV CLONE to build this game against in-progress framework
# work WITHOUT editing the submodule (see the workspace's coord/PROTOCOL.md — the submodule is a
# read-only pinned consumer; framework edits happen in one place):
#
#   cmake -S . -B build -DPSXPORT_DIR=$HOME/repo/psx/psxport
#
# It is a CACHE path, so it survives reconfigures, and `run.sh` passes it through from the
# environment. Everything else in this file and in cmake/ derives from it — never re-spell
# `external/psxport` at a call site, or the two halves of a build can come from different checkouts.
set(PSXPORT_DIR "${CMAKE_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory (default: the external/psxport submodule)")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake — "
                      "run `git submodule update --init --recursive`, or point PSXPORT_DIR at a psxport checkout.")
endif()
message(STATUS "psxport framework: ${PSXPORT_DIR}")

# RECORD WHAT THIS CONFIGURE RESOLVED, so `tools/psxport_sync.py --check` can compare the framework you
# actually BUILT against with the one psxport.pin RECORDS. The framework is no longer a git submodule
# (external/psxport is a symlink to the workspace's shared clone, or a private clone at the pin — see
# tools/psxport_sync.py); the pin is provenance, and this file is what keeps it honest. Measured
# 2026-08-16: this tree was once built against one framework commit while recording another, so a bare
# clone did not compile, and nothing noticed.
execute_process(COMMAND git -C "${PSXPORT_DIR}" rev-parse HEAD
                OUTPUT_VARIABLE PSXPORT_RESOLVED_SHA OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET RESULT_VARIABLE _psxport_rev_rc)
if(NOT _psxport_rev_rc EQUAL 0)
  set(PSXPORT_RESOLVED_SHA "unknown")   # honest: never silently record a plausible-looking sha
endif()
file(WRITE "${CMAKE_BINARY_DIR}/psxport_resolved.txt"
     "dir = ${PSXPORT_DIR}\ncommit = ${PSXPORT_RESOLVED_SHA}\n")
message(STATUS "psxport commit: ${PSXPORT_RESOLVED_SHA}")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ZLIB REQUIRED)
find_package(zstd CONFIG QUIET)
if(NOT TARGET zstd::libzstd_shared)
  find_library(ZSTD_LIBRARY zstd REQUIRED)
  add_library(zstd::libzstd_shared SHARED IMPORTED)
  set_target_properties(zstd::libzstd_shared PROPERTIES IMPORTED_LOCATION "${ZSTD_LIBRARY}")
endif()
find_package(Threads REQUIRED)

# libchdr (CHD access) comes from the Beetle PSX submodule NESTED under the psxport submodule.
add_subdirectory(${PSXPORT_DIR}/vendor/beetle-psx/deps/libchdr ${CMAKE_BINARY_DIR}/libchdr_build EXCLUDE_FROM_ALL)

# The framework static library (psxport) — builds from external/psxport via PSXPORT_ROOT (self-locating).
include(${PSXPORT_DIR}/cmake/psxport.cmake)

# The game binary (tomba2_port): title-native game/** linked to the shared runtime. Guest code is
# read from authenticated user assets and translated at runtime; no emitted guest source is built.
include(cmake/tomba2_port.cmake)
include(cmake/tomba2_tools.cmake)

# Tomba! 1 remains an isolated runtime product while its representative-gameplay evidence is open.
include(titles/tomba1/cmake/tomba1_port.cmake)

if(BUILD_TESTING)
  # Real-title evidence is local-only: CI has no licensed overlay images.
  add_executable(test_authentic_overlay_collision
      tests/test_authentic_overlay_collision.cpp
      tests/authenticated_image.cpp
      game/core/native_override_catalog.cpp)
  target_include_directories(test_authentic_overlay_collision PRIVATE game/core tests)
  target_link_libraries(test_authentic_overlay_collision PRIVATE psxport)

  add_executable(test_authentic_resident_override
      tests/test_authentic_resident_override.cpp
      tests/authenticated_image.cpp
      game/core/native_override_catalog.cpp
      game/core/str.cpp)
  target_include_directories(test_authentic_resident_override PRIVATE game/core tests)
  target_link_libraries(test_authentic_resident_override PRIVATE psxport)

  add_executable(test_native_override_catalog
      tests/test_native_override_catalog.cpp
      game/core/native_override_catalog.cpp
      game/scene/level_load.cpp)
  target_include_directories(test_native_override_catalog PRIVATE game/core game/scene)
  target_link_libraries(test_native_override_catalog PRIVATE psxport)
  add_test(NAME tomba_native_override_catalog COMMAND test_native_override_catalog)

  # DEATH TEST for the issue-0015 abort. `bindResident` now refuses, by aborting, on any resident-form
  # declaration outside the resident text range — the defect that silently kept 42 native owners out
  # of the product. The census above keeps that list empty over the real catalog; this covers the
  # decision itself, which no build-time scan can make, and an untested fatal path is one nobody dares
  # change. The test forks and asserts the child died by SIGABRT, because ctest does not invert
  # WILL_FAIL for a signalled child.
  add_executable(test_native_override_unreachable_abort
      tests/test_native_override_unreachable_abort.cpp
      game/core/native_override_catalog.cpp
      game/scene/level_load.cpp)
  target_include_directories(test_native_override_unreachable_abort PRIVATE game/core game/scene)
  target_link_libraries(test_native_override_unreachable_abort PRIVATE psxport)
  add_test(NAME tomba_native_override_unreachable_abort
           COMMAND test_native_override_unreachable_abort)

  add_executable(test_gpu_timeout_dispatch
      tests/test_gpu_timeout_dispatch.cpp game/core/game_config.cpp)
  target_include_directories(test_gpu_timeout_dispatch PRIVATE game/core)
  target_link_libraries(test_gpu_timeout_dispatch PRIVATE psxport)
  add_test(NAME tomba_gpu_timeout_dispatch COMMAND test_gpu_timeout_dispatch)

  add_executable(test_libapi_vblank_mirror
      tests/test_libapi_vblank_mirror.cpp
      game/core/libapi_intr.cpp
      game/core/native_override_catalog.cpp)
  target_include_directories(test_libapi_vblank_mirror PRIVATE game/core)
  target_link_libraries(test_libapi_vblank_mirror PRIVATE psxport)
  add_test(NAME tomba_libapi_vblank_mirror COMMAND test_libapi_vblank_mirror)

  # The frame-cadence owner (game/core/frame_cadence.h). The gate's own `sltu`, the incrementer's
  # read-modify-write, the u16 wrap, and the accounting denominators. Its measured constants are
  # held to the image by the census below, which is a separate gate on purpose: this one proves the
  # owner behaves, that one proves the owner is describing Tomba! 2.
  add_executable(test_frame_cadence
      tests/test_frame_cadence.cpp
      game/core/frame_cadence.cpp)
  target_include_directories(test_frame_cadence PRIVATE game/core)
  target_link_libraries(test_frame_cadence PRIVATE psxport)
  add_test(NAME tomba_frame_cadence COMMAND test_frame_cadence)

  # The census's own fixtures, through `uv run --frozen` because it is a project tool. It has no
  # game dependency and must pass on a fresh clone with no disc provisioned.
  find_program(TOMBA_UV_CADENCE_UV uv)
  if(TOMBA_UV_CADENCE_UV)
    # The interpolation verdict rule, as a test. Registered because the number it judges is the one
    # the oracle CANNOT see: an in-between never advances guest state, so a leg that reconstructs
    # nothing still compares clean. Without a gate on the rule, "0 unequal at fps60=1" is a claim
    # about a flag rather than about a pipeline.
    add_test(NAME tomba_fps60_interp_selftest
             COMMAND ${TOMBA_UV_CADENCE_UV} run --frozen python
                     ${CMAKE_SOURCE_DIR}/tools/probe_fps60_interp.py --selftest)
    add_test(NAME tomba_frame_cadence_selftest
             COMMAND ${TOMBA_UV_CADENCE_UV} run --frozen python
                     ${CMAKE_SOURCE_DIR}/tools/frame_cadence_census.py --selftest)
    # --check needs the authenticated image, so it SKIPS (77) without one and FAILS with one that
    # disagrees with what game/core/frame_cadence.h ships. Both paths are absolute because CTest
    # runs from the build directory, where a repo-relative one would resolve to nothing and the
    # tool would report a missing image — which is a SKIP, so the gate would look green while
    # checking nothing.
    add_test(NAME tomba_frame_cadence_census
             COMMAND ${TOMBA_UV_CADENCE_UV} run --frozen python
                     ${CMAKE_SOURCE_DIR}/tools/frame_cadence_census.py --check
                     --exe ${CMAKE_SOURCE_DIR}/scratch/bin/tomba2/MAIN.EXE
                     --header ${CMAKE_SOURCE_DIR}/game/core/frame_cadence.h)
    set_tests_properties(tomba_frame_cadence_census PROPERTIES SKIP_RETURN_CODE 77)
  endif()


  add_executable(test_parallax_scroll tests/test_parallax_scroll.cpp)
  target_include_directories(test_parallax_scroll PRIVATE game/scene "${PSXPORT_DIR}/tests")
  add_test(NAME tomba_parallax_scroll COMMAND test_parallax_scroll)

  add_executable(test_ot_key_ord_policy tests/test_ot_key_ord_policy.cpp)
  target_include_directories(test_ot_key_ord_policy PRIVATE game)
  add_test(NAME tomba_ot_key_ord_policy COMMAND test_ot_key_ord_policy)

  add_executable(test_scene_kind tests/test_scene_kind.cpp game/render/scene_kind.cpp)
  target_include_directories(test_scene_kind PRIVATE game/render)
  add_test(NAME tomba_scene_kind COMMAND test_scene_kind)

  add_executable(test_mesh_quads_math tests/test_mesh_quads_math.cpp)
  target_include_directories(test_mesh_quads_math PRIVATE game/render)
  add_test(NAME tomba_mesh_quads_math COMMAND test_mesh_quads_math)

  add_executable(test_prop_quad tests/test_prop_quad.cpp)
  target_include_directories(test_prop_quad PRIVATE game/render)
  add_test(NAME tomba_prop_quad COMMAND test_prop_quad)

  add_executable(test_object_highlight_policy tests/test_object_highlight_policy.cpp)
  target_include_directories(test_object_highlight_policy PRIVATE game/render)
  add_test(NAME tomba_object_highlight_policy COMMAND test_object_highlight_policy)

  add_executable(test_area21_sky_gradient_policy tests/test_area21_sky_gradient_policy.cpp)
  target_include_directories(test_area21_sky_gradient_policy PRIVATE game/render)
  add_test(NAME tomba_area21_sky_gradient_policy COMMAND test_area21_sky_gradient_policy)

  add_executable(test_cube_text_banner_policy tests/test_cube_text_banner_policy.cpp)
  target_include_directories(test_cube_text_banner_policy PRIVATE game/render)
  add_test(NAME tomba_cube_text_banner_policy COMMAND test_cube_text_banner_policy)

  add_executable(test_page_gradient tests/test_page_gradient.cpp game/render/page_gradient.cpp)
  target_include_directories(test_page_gradient PRIVATE game/render)
  add_test(NAME tomba_page_gradient COMMAND test_page_gradient)

  # The horizontal visibility cull, recovered from 29 authenticated images (see
  # game/render/horizontal_visibility_cull.h). 4:3 identity is a SWEEP over the predicate's whole
  # domain, not a sample, and the site->slack table is pinned rather than restated.
  add_executable(test_horizontal_visibility_cull tests/test_horizontal_visibility_cull.cpp)
  target_include_directories(test_horizontal_visibility_cull PRIVATE game/render)
  add_test(NAME tomba_horizontal_visibility_cull COMMAND test_horizontal_visibility_cull)

  # The horizontal-cull BINARY CENSUS, through `uv run --frozen` rather than ${Python3_EXECUTABLE}: the
  # scanner imports capstone, a declared dependency of THIS project's locked environment
  # (pyproject.toml), and CMake's Python3 here is the system interpreter, which has none. A test that
  # only passes on the developer's shell is not a gate. SKIP_RETURN_CODE 77 is what the census exits
  # with when the provisioned images are absent, so a fresh clone without a disc SKIPS instead of
  # failing -- while a PRESENT image that fails to authenticate is a hard failure, which is the whole
  # point of authenticating.
  find_program(TOMBA_UV_EXECUTABLE uv)
  if(TOMBA_UV_EXECUTABLE)
    add_test(NAME tomba_horizontal_cull_selfcheck
             COMMAND ${TOMBA_UV_EXECUTABLE} run --frozen python
                     ${CMAKE_SOURCE_DIR}/tests/horizontal_cull_census.py --selfcheck)
    set_tests_properties(tomba_horizontal_cull_selfcheck PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME tomba_horizontal_cull_census
             COMMAND ${TOMBA_UV_EXECUTABLE} run --frozen python
                     ${CMAKE_SOURCE_DIR}/tests/horizontal_cull_census.py --bodies)
    set_tests_properties(tomba_horizontal_cull_census PROPERTIES SKIP_RETURN_CODE 77)
  endif()

  # The camera driver's mode table: the eighteen driver modes, the render-mode prologue that two of
  # them share, the per-render-mode height floors, and the ten shake states. This table replaced TWO
  # 18-arm switches — one native, one guest-faithful — that could have drifted apart silently. The
  # six distinct (mode, render mode) return constants are the load-bearing part: a body that pushes a
  # frame writes them into guest stack bytes, so collapsing them to one per mode is a behaviour change.
  add_executable(test_camera_mode_table tests/test_camera_mode_table.cpp)
  target_include_directories(test_camera_mode_table PRIVATE game game/camera)
  add_test(NAME tomba_camera_mode_table COMMAND test_camera_mode_table)

  # The cutscene script interpreter's three vocabulary owners: ScriptObject (the driven object's
  # block), the bytecode vocabulary (opcode ids, entry flags, handler returns, cursor moves), and
  # the fixed guest globals. Each makes a decision the raw accessors left to the reader — a field's
  # width and sign, an opcode's id mask, a name this repository has NOT read out of the image — and
  # each of those can be wrong in a way that still runs, which is what these assertions are for.
  # The claim-gate address assertion in particular pins the value the wide RE got 0x60 wrong.
  add_executable(test_script_interp_vocabulary
      tests/test_script_interp_vocabulary.cpp
      game/scene/script_opcode.cpp)
  target_include_directories(test_script_interp_vocabulary PRIVATE game)
  target_link_libraries(test_script_interp_vocabulary PRIVATE psxport)
  add_test(NAME tomba_script_interp_vocabulary COMMAND test_script_interp_vocabulary)

  # Shrink-only legacy caps, all ratcheted down on 2026-09-28.
  #
  # `game/scene/script_interp.cpp` LEFT this list: it was 1317 and the cutscene script interpreter's
  # three vocabulary owners (game/scene/script_object.h, script_opcode.h, script_globals.h) took it to
  # 1196, which is under the 1200 default and needs no cap. A file under the default does not belong in
  # the legacy list — leaving it would be the same "keeps a cap it no longer needs" the gate prevents.
  #
  # `game/core/engine.cpp` LEFT this list too: it was 3251, the largest single file in the repository.
  # `Engine` is still ONE class and game/core/engine.h is still its single declaration — the 47 method
  # bodies moved to six sibling translation units split by the level of the guest's own state machine
  # they serve, with no method body changed. engine.cpp is now the banner and the includes, and every
  # new file is under the 1200 default.
  set(TOMBA_CPP_POLICY_CAPS
      game/player/actor_tomba.cpp=1452
      game/audio/sequencer.cpp=2782
      game/ai/actor_zoned_attacker.cpp=1496
      game/scene/demo.h=87
      game/camera/cutscene_camera.cpp=1459)
  set(TOMBA_CPP_POLICY_ARGS)
  foreach(cap IN LISTS TOMBA_CPP_POLICY_CAPS)
    list(APPEND TOMBA_CPP_POLICY_ARGS --cap "${cap}")
  endforeach()

  add_test(
    NAME tomba_cpp_policy
    COMMAND
      "${Python3_EXECUTABLE}" "${PSXPORT_DIR}/tools/check_cpp_style.py"
      --root "${CMAKE_SOURCE_DIR}"
      --compile-commands "${CMAKE_BINARY_DIR}"
      ${TOMBA_CPP_POLICY_ARGS})
  add_test(
    NAME tomba_launcher_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/test_run.py")
  add_test(
    NAME tomba2_provision_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/test_tomba2_provision.py")
  add_test(
    NAME tomba_psxport_pin_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/test_psxport_sync.py")
  # THE LIVE CHECK, and it is a different test from the selftest above.
  #
  # MEASURED 2026-09-27: every port's "pin test" was a selftest of the check FUNCTION, run against a
  # temporary fixture. `tomba_psxport_pin_selftest` can pass 14/14 while this repository's actual
  # `psxport.pin` names a framework commit nothing here was built against, because the selftest never
  # reads the real pin or the real receipt. A gate that cannot fail on the thing it gates is not a gate,
  # and that is why all ten ports showed green while four of their pins were demonstrably stale.
  #
  # This test runs the real function against the real pin and the real receipt, and it points
  # `--build` at THIS build directory — the one CTest is running in, which was configured moments ago, so
  # its receipt is current by construction and the staleness guard cannot fire spuriously.
  add_test(
    NAME tomba_psxport_pin
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/psxport_sync.py"
            --check --build "${CMAKE_BINARY_DIR}")
  add_test(
    NAME tomba_dynarec_boundary_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_dynarec_boundary.py" --selftest)
  add_test(
    NAME tomba_native_frame_contract_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_native_frame_contract.py" --selftest)
  add_test(
    NAME tomba_native_frame_contract
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_native_frame_contract.py")
  # The band measure's own discriminator. It is registered rather than left to a manual run because
  # its FIRST version reported a band on an almost-empty page (it compared each row against the
  # picture's MEDIAN row, and a page whose median is 0 makes every non-empty row a "band"), and a
  # measure that can only answer "yes" is not a measure. The negative cases are the ones that hold it
  # to that.
  add_test(
    NAME tomba_aspect_pair_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/aspect_pair.py" --selftest)
  # THE CONVERSION GATE for issue 0015, and the check that makes a batch conversion safe rather than a
  # guess: it cross-checks EVERY overlay-form declaration against the authenticated MODE images and
  # exits 1 on any mismatch, so a declaration naming an image that cannot install it is a BUILD failure
  # instead of a behaviour that is silently absent from the product.
  #
  # 32 native owners were dead for exactly this reason — declared at overlay addresses with the
  # resident form, so `bindResident` could never install them. The census's counts were validated
  # against a recorded pre-change product run (`bound overlay 'A00' ... installed=10`), and this test
  # keeps that agreement honest on every build: SOP 3/3 and A00 39/39 declared and in range.
  #
  # It reads the extracted overlay images under scratch/bin/overlays, which are PROVISIONED INPUTS, so
  # this test refuses rather than passing vacuously when they are absent.
  add_test(
    NAME tomba_overlay_owner_census
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/overlay_owner_map.py" --census)
endif()
