add_subdirectory(helpers)

set(CLIENT_HEADERS
    accessproxy.h
    api_util_templ.h
    apiexcept.h
    ApiUtil.h
    AttributeProxy.h
    cbthread.h
    Connection.h
    Database.h
    dbapi.h
    DbDevice.h
    devapi_attr_templ.h
    devapi_pipe_templ.h
    devapi_utils_templ.h
    devapi.h
    devasyn.h
    DeviceAttribute.h
    DeviceData.h
    DevicePipe.h
    DeviceProxy.h
    event.h
    event_templ.h
    eventconsumer.h
    filedatabase.h
    group.h
    lockthread.h
)

install(FILES ${CLIENT_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tango/client")
