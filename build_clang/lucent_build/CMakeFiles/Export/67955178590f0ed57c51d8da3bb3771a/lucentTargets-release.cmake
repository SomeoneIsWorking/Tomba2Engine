#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "lucent::lucent" for configuration "Release"
set_property(TARGET lucent::lucent APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(lucent::lucent PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib64/liblucent.a"
  )

list(APPEND _cmake_import_check_targets lucent::lucent )
list(APPEND _cmake_import_check_files_for_lucent::lucent "${_IMPORT_PREFIX}/lib64/liblucent.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
