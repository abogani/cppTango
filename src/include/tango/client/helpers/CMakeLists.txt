set(CLIENT_HEADERS_HELPERS DeviceProxyHelper.h
    PogoHelper.h
    TangoExceptionsHelper.h
    Xstring.h)

install(FILES ${CLIENT_HEADERS_HELPERS} DESTINATION
    "${CMAKE_INSTALL_INCLUDEDIR}/tango/client/helpers")
