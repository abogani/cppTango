add_subdirectory(utils)

configure_file(tango_const.h.in tango_const.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tango_const.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tango/common")
