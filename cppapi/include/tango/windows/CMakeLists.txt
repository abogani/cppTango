set(WINDOWS_HEADERS coutbuf.h
    ntservice.h
    resource.h
    w32win.h)

install(FILES ${WINDOWS_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tango/windows")
