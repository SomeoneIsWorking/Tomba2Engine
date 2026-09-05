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
# or anyone cloning this repo alone). tools/psxport_sync.py --auto establishes whichever applies, and
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
  add_executable(test_native_override_catalog
      tests/test_native_override_catalog.cpp game/core/native_override_catalog.cpp)
  target_include_directories(test_native_override_catalog PRIVATE game/core)
  target_link_libraries(test_native_override_catalog PRIVATE psxport)
  add_test(NAME tomba_native_override_catalog COMMAND test_native_override_catalog)

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

  set(TOMBA_CPP_POLICY_CAPS
      game/core/engine.cpp=4375
      game/player/actor_tomba.cpp=1452
      game/audio/sequencer.cpp=2796
      game/ai/actor_zoned_attacker.cpp=1535
      game/scene/demo.h=87
      game/camera/cutscene_camera.cpp=1552
      game/scene/script_interp.cpp=1317)
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
    NAME tomba_dynarec_boundary_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_dynarec_boundary.py" --selftest)
  add_test(
    NAME tomba_native_frame_contract_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_native_frame_contract.py" --selftest)
  add_test(
    NAME tomba_native_frame_contract
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/verify_native_frame_contract.py")
endif()
