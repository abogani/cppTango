add_subdirectory(idl)
add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)

# the folder internal is intentionally left out

if(WIN32)
    add_subdirectory(windows)
endif()

install(FILES tango.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tango")
