#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "KazDragon::telnetpp" for configuration "Release"
set_property(TARGET KazDragon::telnetpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(KazDragon::telnetpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/telnetpp-0.0/libtelnetpp.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS KazDragon::telnetpp )
list(APPEND _IMPORT_CHECK_FILES_FOR_KazDragon::telnetpp "${_IMPORT_PREFIX}/lib/telnetpp-0.0/libtelnetpp.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
