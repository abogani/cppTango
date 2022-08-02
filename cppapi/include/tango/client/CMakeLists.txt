add_subdirectory(helpers)

set(CLIENT_HEADERS
    accessproxy.h
    api_util.tpp
    apiexcept.h
    ApiUtil.h
    AttributeProxy.h
    cbthread.h
    Connection.h
    Database.h
    dbapi.h
    DbDevice.h
    devapi_attr.tpp
    devapi_pipe.tpp
    devapi_utils.tpp
    devapi.h
    devasyn.h
    DeviceAttribute.h
    DeviceData.h
    DevicePipe.h
    DeviceProxy.h
    event.h
    event.tpp
    eventconsumer.h
    filedatabase.h
    group.h
    lockthread.h
)

install(FILES ${CLIENT_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tango/client")
