set_target_properties(tango
    PROPERTIES
    EXPORT_NAME Tango
    )

configure_file(tango.pc.cmake tango.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tango.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

# Install Config -----------------------------------
include(CMakePackageConfigHelpers)

set(ConfigPackageLocation "${CMAKE_INSTALL_LIBDIR}/cmake/tango")
set(Namespace Tango::)

# write tango version information
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/tango/TangoConfigVersion.cmake"
  VERSION ${LIBRARY_VERSION}
  COMPATIBILITY SameMinorVersion
)

# export the targets built
export(EXPORT TangoTargets
  FILE
    "${CMAKE_CURRENT_BINARY_DIR}/tango/TangoTargets.cmake"
  NAMESPACE ${Namespace}
)
# generate the config file that includes the exports
configure_package_config_file(configure/TangoConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/tango/TangoConfig.cmake"
  INSTALL_DESTINATION
    ${ConfigPackageLocation}
  NO_SET_AND_CHECK_MACRO
)

# install the exported targets
install(EXPORT TangoTargets
  FILE
    TangoTargets.cmake
  NAMESPACE
    ${Namespace}
  DESTINATION
    ${ConfigPackageLocation}
)

# install the cmake files
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/tango/TangoConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/tango/TangoConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
)

# install the cmake find modules
install(
  DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/${TANGO_FIND_MODULES_PATH}"
  DESTINATION
    ${ConfigPackageLocation}
  FILES_MATCHING PATTERN "*.cmake"
  PATTERN "FindTango.cmake" EXCLUDE
)
if (TANGO_USE_JPEG)
  install(
    FILES
      "${CMAKE_CURRENT_LIST_DIR}/FindJPEG.cmake"
    DESTINATION
      ${ConfigPackageLocation}/${TANGO_FIND_MODULES_PATH}
  )
endif()
