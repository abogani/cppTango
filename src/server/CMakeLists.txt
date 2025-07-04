set(SOURCES attrdesc.cpp
            attrgetsetprop.cpp
            attribute.cpp
            attribute_utils.cpp
            attrsetval.cpp
            attrmanip.cpp
            auto_tango_monitor.cpp
            basiccommand.cpp
            blackbox.cpp
            class_factory.cpp
            classattribute.cpp
            command.cpp
            coutappender.cpp
            classpipe.cpp
            dev_event.cpp
            dev_poll.cpp
            device.cpp
            device_2.cpp
            device_3.cpp
            device_4.cpp
            device_5.cpp
            device_6.cpp
            deviceclass.cpp
            devicelog.cpp
            devicetelemetry.cpp
            devintr.cpp
            dintrthread.cpp
            dserver.cpp
            dserverclass.cpp
            dserverlock.cpp
            dserverlog.cpp
            dserverpoll.cpp
            dserversignal.cpp
            encoded_attribute.cpp
            eventcmds.cpp
            eventsupplier.cpp
            except.cpp
            fwdattrdesc.cpp
            fwdattribute.cpp
            logcmds.cpp
            logging.cpp
            logstream.cpp
            multiattribute.cpp
            notifdeventsupplier.cpp
            pipe.cpp
            pollcmds.cpp
            pollobj.cpp
            pollring.cpp
            pollthread.cpp
            rootattreg.cpp
            seqvec.cpp
            subdev_diag.cpp
            tangoappender.cpp
            tangorollingfileappender.cpp
            templ_inst.cpp
            thsig.cpp
            utils.cpp
            utils_polling.cpp
            utils_shut.cpp
            w_attribute.cpp
            w_pipe.cpp
            zmqeventsupplier.cpp)

if(WIN32)
    set(SOURCES_WIN
        coutbuf.cpp
        ntservice.cpp
        w32win.cpp)

    list(APPEND SOURCES ${SOURCES_WIN})
endif()

add_library(server_objects OBJECT ${SOURCES})
add_dependencies(server_objects idl_objects common_objects)
set_cflags_and_include(server_objects)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set_source_files_properties(
    deviceclass.cpp
    PROPERTIES
    COMPILE_FLAGS -Wno-stringop-overflow)
endif()

if(WIN32)
    target_compile_definitions(server_objects PRIVATE "${windows_defs}")
else()
    target_compile_options(server_objects PRIVATE -fPIC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Do not define _TANGO_LIB when compiling Tango debug library on Linux
        # in order to keep the same behaviour as in the past:
        # A Linux Tango Device Server using the debug tango lib will display
        # twice "Ready to accept requests" at startup
    else()
        target_compile_definitions(server_objects PRIVATE _TANGO_LIB)
    endif()
endif()

target_precompile_headers(server_objects REUSE_FROM common_objects)
