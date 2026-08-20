# First-party offline tools remain outside the default game build, but each is a real Clang target.
# That keeps their compile commands authoritative for clang-tidy and prevents source-only fossils.
if(NOT TARGET psxport_common)
  add_subdirectory("${PSXPORT_DIR}/common" "${CMAKE_BINARY_DIR}/psxport_common_build" EXCLUDE_FROM_ALL)
endif()

add_executable(menu_bg_export EXCLUDE_FROM_ALL tools/menu_bg_export.cpp)
target_link_libraries(menu_bg_export PRIVATE psxport psxport_common ZLIB::ZLIB)

add_executable(dump_all_seq EXCLUDE_FROM_ALL tools/dump_all_seq.cpp)
target_link_libraries(dump_all_seq PRIVATE psxport psxport_common)

foreach(tool IN ITEMS render_all_seq render_one)
  add_executable(${tool} EXCLUDE_FROM_ALL tools/${tool}.c game/audio/native_audio.c)
  target_include_directories(${tool} PRIVATE game)
  target_link_libraries(${tool} PRIVATE m)
endforeach()

set_target_properties(
  menu_bg_export dump_all_seq render_all_seq render_one
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/scratch/bin")
