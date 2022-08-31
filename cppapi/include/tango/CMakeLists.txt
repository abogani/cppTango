add_subdirectory(idl)
add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)

if(WIN32)
    add_subdirectory(windows)
endif()

set(TANGO_H_HEADER tango.h)
install(FILES ${TANGO_H_HEADER} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tango")
