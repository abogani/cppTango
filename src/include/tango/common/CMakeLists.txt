add_subdirectory(utils)

configure_file(tango_version.h.in tango_version.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tango_version.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tango/common")
