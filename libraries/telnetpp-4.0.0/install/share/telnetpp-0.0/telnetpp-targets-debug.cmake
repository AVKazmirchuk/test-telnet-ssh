#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "KazDragon::telnetpp" for configuration "Debug"
set_property(TARGET KazDragon::telnetpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(KazDragon::telnetpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/telnetpp-0.0/libtelnetppd.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS KazDragon::telnetpp )
list(APPEND _IMPORT_CHECK_FILES_FOR_KazDragon::telnetpp "${_IMPORT_PREFIX}/lib/telnetpp-0.0/libtelnetppd.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
