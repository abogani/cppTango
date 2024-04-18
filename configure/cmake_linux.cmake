set_target_properties(
    log4tango_objects client_objects idl_objects common_objects
    PROPERTIES
    UNITY_BUILD_CODE_BEFORE_INCLUDE "// NOLINTNEXTLINE(bugprone-suspicious-include)")

set_target_properties(tango
    PROPERTIES
    EXPORT_NAME Tango
    )

if(BUILD_SHARED_LIBS)
target_compile_options(tango PRIVATE -fPIC)
set_target_properties(tango PROPERTIES
                    VERSION ${LIBRARY_VERSION}
                    SOVERSION ${SO_VERSION})
endif()

# Install Config -----------------------------------
include(CMakePackageConfigHelpers)

install(
TARGETS tango
    EXPORT TangoTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

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

configure_file(tango.pc.cmake tango.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tango.pc"
        DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")

include(configure/cpack_linux.cmake)
