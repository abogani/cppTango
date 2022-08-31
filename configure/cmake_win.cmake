project(tango)

if(CMAKE_CL_64)
    add_definitions(-D_64BITS)
endif()

# multi process compilation
add_compile_options(/MP)

set(TANGO_LIBRARY_NAME tango)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TANGO_LIBRARY_NAME ${TANGO_LIBRARY_NAME}d)
endif()

# The name without the variant tag (i.e. -static)
set(TANGO_LIBRARY_OUTPUT_NAME ${TANGO_LIBRARY_NAME})

if(BUILD_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    set(TANGO_LIBRARY_NAME ${TANGO_LIBRARY_NAME}-static)
endif()

message("Tango library is '${TANGO_LIBRARY_NAME}'")

#dump version into a file, later used by AppVeyor
file(WRITE ${PROJECT_BINARY_DIR}/VERSION ${LIBRARY_VERSION})

#include and link directories

include_directories(SYSTEM ${PTHREAD_WIN_PKG_INCLUDE_DIRS})
set(WIN32_LIBS "ws2_32.lib;mswsock.lib;advapi32.lib;comctl32.lib;odbc32.lib;")
if(PTHREAD_WIN)
    link_directories(${PTHREAD_WIN}/lib)
endif()

add_library(${TANGO_LIBRARY_NAME} $<TARGET_OBJECTS:log4tango_objects>
        $<TARGET_OBJECTS:idl_objects>
        $<TARGET_OBJECTS:client_objects>
        $<TARGET_OBJECTS:server_objects>)

set_target_properties(${TANGO_LIBRARY_NAME} PROPERTIES COMPILE_DEFINITIONS
        "${windows_defs}")

target_compile_options(${TANGO_LIBRARY_NAME} PUBLIC ${ZMQ_PKG_CFLAGS_OTHER} ${OMNIORB_PKG_CFLAGS_OTHER} ${OMNICOS_PKG_CFLAGS_OTHER} ${OMNIDYN_PKG_CFLAGS_OTHER})

if(BUILD_SHARED_LIBS)
    target_link_libraries(${TANGO_LIBRARY_NAME} PUBLIC ${WIN32_LIBS} ${OMNIORB_PKG_LIBRARIES_DYN} ${ZMQ_PKG_LIBRARIES_DYN} ${PTHREAD_WIN_PKG_LIBRARIES_DYN} ${CMAKE_DL_LIBS} PRIVATE ${JPEG_PKG_LIBRARIES_DYN})
else()
    target_link_libraries(${TANGO_LIBRARY_NAME} PUBLIC ${WIN32_LIBS} ${OMNIORB_PKG_LIBRARIES_STA} ${ZMQ_PKG_LIBRARIES_STA} ${PTHREAD_WIN_PKG_LIBRARIES_STA} ${CMAKE_DL_LIBS} PRIVATE ${JPEG_PKG_LIBRARIES_STA})
    set_target_properties(${TANGO_LIBRARY_NAME} PROPERTIES OUTPUT_NAME ${TANGO_LIBRARY_OUTPUT_NAME})
    set_target_properties(${TANGO_LIBRARY_NAME} PROPERTIES PREFIX "lib")
endif()

# Always generate separate PDB files for shared builds, even for release build types
#
# https://docs.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format
# https://docs.microsoft.com/en-us/cpp/build/reference/debug-generate-debug-info
target_compile_options(${TANGO_LIBRARY_NAME} PRIVATE "/Zi")
set_property(TARGET ${TANGO_LIBRARY_NAME} PROPERTY LINK_FLAGS "/force:multiple /DEBUG")

target_include_directories(${TANGO_LIBRARY_NAME} SYSTEM PUBLIC ${ZMQ_PKG_INCLUDE_DIRS} ${OMNIORB_PKG_INCLUDE_DIRS} ${OMNIDYN_PKG_INCLUDE_DIRS})

set_target_properties(${TANGO_LIBRARY_NAME} PROPERTIES
        VERSION ${LIBRARY_VERSION}
        SOVERSION ${SO_VERSION})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
    set(CMAKE_INSTALL_CONFIG_NAME Debug)
endif()

#install code

install(TARGETS ${TANGO_LIBRARY_NAME}
        ARCHIVE DESTINATION lib COMPONENT static
        RUNTIME DESTINATION bin COMPONENT dynamic)

install(DIRECTORY "$<TARGET_FILE_DIR:${TANGO_LIBRARY_NAME}>/"
        DESTINATION lib COMPONENT static
        DESTINATION bin COMPONENT dynamic
        FILES_MATCHING PATTERN "*.pdb")

if (TANGO_INSTALL_DEPENDENCIES)
    install(DIRECTORY ${TANGO_OMNI_BASE}/include/COS DESTINATION include COMPONENT)
    install(DIRECTORY ${TANGO_OMNI_BASE}/include/omniORB4 DESTINATION include COMPONENT headers)
    install(DIRECTORY ${TANGO_OMNI_BASE}/include/omnithread DESTINATION include COMPONENT headers FILES_MATCHING PATTERN "*.h" PATTERN "*.in" EXCLUDE)
    install(DIRECTORY ${TANGO_OMNI_BASE}/include/omniVms DESTINATION include COMPONENT headers)
    install(FILES ${TANGO_OMNI_BASE}/include/omniconfig.h DESTINATION include COMPONENT headers)
    install(FILES ${TANGO_OMNI_BASE}/include/omnithread.h DESTINATION include COMPONENT headers)
    install(FILES ${TANGO_ZMQ_BASE}/include/zmq.h DESTINATION include COMPONENT headers)
    install(FILES ${TANGO_ZMQ_BASE}/include/zmq_utils.h DESTINATION include COMPONENT headers)
    install(FILES ${TANGO_CPPZMQ_BASE}/include/zmq.hpp DESTINATION include COMPONENT headers)
    install(FILES ${TANGO_CPPZMQ_BASE}/include/zmq_addon.hpp DESTINATION include COMPONENT headers)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        #omniorb static lib
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniDynamic4d.lib DESTINATION lib COMPONENT static)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniORB4d.lib DESTINATION lib COMPONENT static)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniDynamic4d.lib DESTINATION lib COMPONENT static)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omnithreadd.lib DESTINATION lib COMPONENT static)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/COS4d.lib DESTINATION lib COMPONENT static)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniORB4_rtd.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniDynamic4_rtd.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omnithread_rtd.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/COS4_rtd.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniORB421_rtd.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniDynamic421_rtd.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omnithread40_rtd.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/COS421_rtd.lib DESTINATION lib COMPONENT dynamic)

    else()

        #omniorb static lib
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniORB4.lib DESTINATION lib COMPONENT static)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniDynamic4.lib DESTINATION lib COMPONENT static)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omnithread.lib DESTINATION lib COMPONENT static)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/COS4.lib DESTINATION lib COMPONENT static)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniORB4_rt.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniDynamic4_rt.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omnithread_rt.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/COS4_rt.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniORB421_rt.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omniDynamic421_rt.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/omnithread40_rt.lib DESTINATION lib COMPONENT dynamic)
        install(FILES ${TANGO_OMNI_BASE}/lib/x86_win32/COS421_rt.lib DESTINATION lib COMPONENT dynamic)

    endif()

    if(CMAKE_VS_PLATFORM_TOOLSET IN_LIST WINDOWS_SUPPORTED_VS_TOOLSETS)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            install(FILES ${TANGO_OMNI_BASE}/bin/x86_win32/omniORB421_vc15_rtd.dll DESTINATION bin COMPONENT dynamic)
            install(FILES ${TANGO_OMNI_BASE}/bin/x86_win32/omniDynamic421_vc15_rtd.dll DESTINATION bin COMPONENT dynamic)
            install(FILES ${TANGO_OMNI_BASE}/bin/x86_win32/omnithread40_vc15_rtd.dll DESTINATION bin COMPONENT dynamic)
            install(FILES ${TANGO_OMNI_BASE}/bin/x86_win32/COS421_vc15_rtd.dll DESTINATION bin COMPONENT dynamic)
            install(FILES ${TANGO_ZMQ_BASE}/lib/Debug/libzmq-v141-mt-gd-4_0_5.lib DESTINATION lib COMPONENT static)
            install(FILES ${TANGO_ZMQ_BASE}/lib/Debug/libzmq-v141-mt-sgd-4_0_5.lib DESTINATION lib COMPONENT static)
            install(FILES ${TANGO_ZMQ_BASE}/bin/Debug/libzmq-v141-mt-gd-4_0_5.dll DESTINATION bin COMPONENT dynamic)
        else()
            install(FILES ${TANGO_OMNI_BASE}/bin/x86_win32/omniORB421_vc15_rt.dll DESTINATION bin COMPONENT dynamic)
            install(FILES ${TANGO_OMNI_BASE}/bin/x86_win32/omniDynamic421_vc15_rt.dll DESTINATION bin COMPONENT dynamic)
            install(FILES ${TANGO_OMNI_BASE}/bin/x86_win32/omnithread40_vc15_rt.dll DESTINATION bin COMPONENT dynamic)
            install(FILES ${TANGO_OMNI_BASE}/bin/x86_win32/COS421_vc15_rt.dll DESTINATION bin COMPONENT dynamic)
            install(FILES ${TANGO_ZMQ_BASE}/lib/Release/libzmq-v141-mt-4_0_5.lib DESTINATION lib COMPONENT static)
            install(FILES ${TANGO_ZMQ_BASE}/lib/Release/libzmq-v141-mt-s-4_0_5.lib DESTINATION lib COMPONENT static)
            install(FILES ${TANGO_ZMQ_BASE}/bin/Release/libzmq-v141-mt-4_0_5.dll DESTINATION bin COMPONENT dynamic)
        endif()
    endif()

    #pthreads
    if (PTHREAD_WIN)
        install(FILES ${PTHREAD_WIN}/lib/pthreadVC2.lib DESTINATION lib COMPONENT static)
        install(FILES ${PTHREAD_WIN}/lib/pthreadVC2-s.lib DESTINATION lib COMPONENT static)
        install(FILES ${PTHREAD_WIN}/bin/pthreadVC2.dll DESTINATION bin COMPONENT dynamic)
        install(FILES ${PTHREAD_WIN}/bin/pthreadVC2.pdb DESTINATION bin COMPONENT dynamic)
        install(FILES ${PTHREAD_WIN}/bin/pthreadVC2.exp DESTINATION bin COMPONENT dynamic)
        install(FILES ${PTHREAD_WIN}/bin/pthreadVC2.ilk DESTINATION bin COMPONENT dynamic)
        install(FILES ${PTHREAD_WIN}/lib/pthreadVC2d.lib DESTINATION lib COMPONENT static)
        install(FILES ${PTHREAD_WIN}/lib/pthreadVC2-sd.lib DESTINATION lib COMPONENT static)
        install(FILES ${PTHREAD_WIN}/bin/pthreadVC2d.dll DESTINATION bin COMPONENT dynamic)
        install(FILES ${PTHREAD_WIN}/bin/pthreadVC2d.pdb DESTINATION bin COMPONENT dynamic)
        install(FILES ${PTHREAD_WIN}/bin/pthreadVC2d.exp DESTINATION bin COMPONENT dynamic)
        install(FILES ${PTHREAD_WIN}/bin/pthreadVC2d.ilk DESTINATION bin COMPONENT dynamic)
    endif()

    #Jpeg
    if (TANGO_USE_JPEG)
        install(FILES ${JPEG_DLL} DESTINATION lib COMPONENT dynamic)
    endif()
endif()
