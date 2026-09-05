# Tomba! 1 runtime product and evidence CMake fragment. Identity, provisioning, isolation, and
# binary-backed comparison tools remain available.

get_filename_component(TOMBA1_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(TOMBA1_EXECUTABLE
    "${CMAKE_SOURCE_DIR}/scratch/bin/tomba1/SCUS_942.36"
    CACHE FILEPATH "Identity-checked Tomba! 1 USA executable")

if(NOT TARGET psxport)
  message(FATAL_ERROR "Tomba! 1 scaffold requires the shared psxport library target")
endif()
if(NOT TARGET oracle_trace)
  add_subdirectory(
    "${PSXPORT_DIR}/tools/oracle"
    "${CMAKE_BINARY_DIR}/tomba1_oracle_build"
    EXCLUDE_FROM_ALL)
endif()

add_custom_target(tomba1_scaffold DEPENDS psxport)

add_library(
  tomba1_runtime STATIC
  "${TOMBA1_ROOT}/game/core/cd_native_startup.cpp"
  "${TOMBA1_ROOT}/game/core/frame_driver.cpp"
  "${TOMBA1_ROOT}/game/core/native_boot.cpp"
  "${TOMBA1_ROOT}/game/core/stream_field_turn.cpp"
  "${TOMBA1_ROOT}/game/core/sync_native.cpp"
  "${TOMBA1_ROOT}/game/core/tomba1_runtime.cpp")
set_target_properties(tomba1_runtime PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_include_directories(tomba1_runtime PUBLIC "${TOMBA1_ROOT}/game/core")
target_link_libraries(tomba1_runtime PUBLIC psxport)

add_executable(tomba1_port "${TOMBA1_ROOT}/game/app/main.cpp")
set_target_properties(
  tomba1_port
  PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries(tomba1_port PRIVATE tomba1_runtime)

if(BUILD_TESTING)
  add_executable(
    tomba1_stream_field_turn_test
    "${TOMBA1_ROOT}/tests/test_stream_field_turn.cpp")
  set_target_properties(
    tomba1_stream_field_turn_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(tomba1_stream_field_turn_test PRIVATE tomba1_runtime)
  add_test(NAME tomba1_stream_field_turn COMMAND tomba1_stream_field_turn_test)
endif()

add_custom_target(
  tomba1_identity_selftest
  COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_executable.py" --selftest
  USES_TERMINAL
  VERBATIM)

add_custom_target(
  tomba1_identity_check
  COMMAND
    "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_executable.py"
    --check "${TOMBA1_EXECUTABLE}"
  USES_TERMINAL
  VERBATIM)

add_custom_target(
  tomba1_provision_selftest
  COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tests/test_provision.py"
  USES_TERMINAL
  VERBATIM)

add_custom_target(
  tomba1_crt0_compare_selftest
  COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/compare_crt0_boundary.py" --selftest
  USES_TERMINAL
  VERBATIM)

add_custom_target(
  tomba1_structure_check
  COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_title_isolation.py"
  USES_TERMINAL
  VERBATIM)

add_custom_target(
  tomba1_structure_selftest
  COMMAND
    "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_title_isolation.py"
    --selftest
  USES_TERMINAL
  VERBATIM)

add_custom_target(
  tomba1_verify_scaffold
  DEPENDS
    tomba1_scaffold
    tomba1_identity_selftest
    tomba1_crt0_compare_selftest
    tomba1_provision_selftest
    tomba1_structure_check
    tomba1_structure_selftest)

if(BUILD_TESTING)
  add_test(
    NAME tomba1_identity_selftest
    COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_executable.py" --selftest)
  add_test(
    NAME tomba1_provision_selftest
    COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tests/test_provision.py")
  add_test(
    NAME tomba1_crt0_compare_selftest
    COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/compare_crt0_boundary.py" --selftest)
  add_test(
    NAME tomba1_title_isolation
    COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_title_isolation.py")
  add_test(
    NAME tomba1_title_isolation_selftest
    COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_title_isolation.py"
            --selftest)
endif()
