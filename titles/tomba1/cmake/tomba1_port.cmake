# Tomba! 1 harness-first CMake fragment.
#
# This file is included by the repository root and publishes only evidence gates. A real tomba1_port
# target belongs here after the first generated/oracle boundary exists;
# publishing an executable before then would disguise an absent execution path as a product.

get_filename_component(TOMBA1_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(TOMBA1_EXECUTABLE
    "${CMAKE_SOURCE_DIR}/scratch/bin/tomba1/SCUS_942.36"
    CACHE FILEPATH "Identity-checked Tomba! 1 USA executable")

if(NOT TARGET psxport)
  message(FATAL_ERROR "Tomba! 1 scaffold requires the shared psxport library target")
endif()

add_custom_target(tomba1_scaffold DEPENDS psxport)

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
    tomba1_structure_check
    tomba1_structure_selftest)

if(BUILD_TESTING)
  add_test(
    NAME tomba1_identity_selftest
    COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_executable.py" --selftest)
  add_test(
    NAME tomba1_title_isolation
    COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_title_isolation.py")
  add_test(
    NAME tomba1_title_isolation_selftest
    COMMAND "${Python3_EXECUTABLE}" "${TOMBA1_ROOT}/tools/verify_title_isolation.py"
            --selftest)
endif()
