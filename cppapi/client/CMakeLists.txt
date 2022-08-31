set(SOURCES dbapi_class.cpp
            dbapi_server.cpp
            dbapi_datum.cpp
            dbapi_base.cpp
            dbapi_device.cpp
            dbapi_history.cpp
            dbapi_attribute.cpp
            dbapi_cache.cpp
            dbapi_serverdata.cpp
            devapi_attr.cpp
            devapi_base.cpp
            devapi_data.cpp
            devapi_datahist.cpp
            devapi_utils.cpp
            devapi_pipe.cpp
            api_util.cpp
            asynreq.cpp
            cbthread.cpp
            proxy_asyn.cpp
            proxy_asyn_cb.cpp
            attr_proxy.cpp
            group.cpp
            filedatabase.cpp
            apiexcept.cpp
            accessproxy.cpp
            lockthread.cpp
            event.cpp
            eventkeepalive.cpp
            eventqueue.cpp
            notifdeventconsumer.cpp
            zmqeventconsumer.cpp)

add_library(client_objects OBJECT ${SOURCES})
add_dependencies(client_objects idl_objects)

if(WIN32)
    target_compile_definitions(client_objects PRIVATE "${windows_defs}")

    if(TANGO_USE_PCH AND NOT BUILD_SHARED_LIBS)
        # shared cannot be compiled with PCH due to __@_PchSym_ collision
        tango_target_pch(client_objects)
    endif()
else()
    target_compile_options(client_objects PRIVATE -fPIC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Do not define _TANGO_LIB when compiling Tango debug library on Linux
        # in order to keep the same behaviour as in the past:
        # cout messages are not redirected to the logging system but are
        # instead displayed directly on the console
        target_compile_definitions(client_objects PRIVATE _REENTRANT)
    else()
        target_compile_definitions(client_objects PRIVATE _REENTRANT _TANGO_LIB)
    endif()
    if(TANGO_USE_PCH)
        tango_target_pch(client_objects)
    endif()
endif()
