set_target_properties(
    log4tango_objects client_objects idl_objects common_objects
    PROPERTIES
    UNITY_BUILD_CODE_BEFORE_INCLUDE "// NOLINTNEXTLINE(bugprone-suspicious-include)")

if(BUILD_SHARED_LIBS)
  target_compile_options(tango PRIVATE -fPIC)
  set_target_properties(tango PROPERTIES
                        VERSION ${LIBRARY_VERSION}
                        SOVERSION ${SO_VERSION})
  install(TARGETS tango LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
else()
  install(TARGETS tango ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()

include(configure/cpack_linux.cmake)
