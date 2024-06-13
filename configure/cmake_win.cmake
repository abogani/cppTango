if(CMAKE_CL_64)
    add_definitions(-D_64BITS)
endif()

# multi process compilation
add_compile_options(/MP)

set(TANGO_LIBRARY_NAME tango)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TANGO_LIBRARY_NAME ${TANGO_LIBRARY_NAME}d)
endif()

if(BUILD_SHARED_LIBS)
    set_target_properties(tango PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    set(TANGO_LIBRARY_NAME ${TANGO_LIBRARY_NAME}-static)
endif()

message("Tango library is '${TANGO_LIBRARY_NAME}'")

#include and link directories

set(WIN32_LIBS "ws2_32.lib;mswsock.lib;advapi32.lib;comctl32.lib;odbc32.lib;")

set_target_properties(tango PROPERTIES
    COMPILE_DEFINITIONS "${windows_defs}"
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${SO_VERSION}
    DEBUG_POSTFIX "d")

set_cflags_and_include(tango)

if(NOT BUILD_SHARED_LIBS)
    set_target_properties(tango PROPERTIES
        PREFIX "lib"
        OUTPUT_NAME ${TANGO_LIBRARY_NAME})
endif()

# Always generate separate PDB files for shared builds, even for release build types
#
# https://docs.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format
# https://docs.microsoft.com/en-us/cpp/build/reference/debug-generate-debug-info
target_compile_options(tango PRIVATE "/Zi")

target_link_libraries(tango
    PRIVATE
        ${WIN32_LIBS})

if(TANGO_USE_PTHREAD)
    target_link_libraries(tango
        PRIVATE
            pthread::pthread)
endif()

set_property(TARGET tango PROPERTY LINK_FLAGS "/force:multiple /DEBUG")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_INSTALL_CONFIG_NAME Debug)
endif()

#install code

install(TARGETS tango
        EXPORT TangoTargets
        ARCHIVE DESTINATION lib COMPONENT static
        RUNTIME DESTINATION bin COMPONENT dynamic)

install(DIRECTORY "$<TARGET_FILE_DIR:tango>/"
        DESTINATION lib COMPONENT static
        DESTINATION bin COMPONENT dynamic
        FILES_MATCHING PATTERN "*.pdb")

if (TANGO_INSTALL_DEPENDENCIES)
    install(DIRECTORY ${omniORB4_INCLUDE_DIR}/COS DESTINATION include COMPONENT)
    install(DIRECTORY ${omniORB4_INCLUDE_DIR}/omniORB4 DESTINATION include COMPONENT headers)
    install(DIRECTORY ${omniORB4_INCLUDE_DIR}/omnithread DESTINATION include COMPONENT headers FILES_MATCHING PATTERN "*.h" PATTERN "*.in" EXCLUDE)
    install(DIRECTORY ${omniORB4_INCLUDE_DIR}/omniVms DESTINATION include COMPONENT headers)
    install(FILES ${omniORB4_INCLUDE_DIR}/omniconfig.h DESTINATION include COMPONENT headers)
    install(FILES ${omniORB4_INCLUDE_DIR}/omnithread.h DESTINATION include COMPONENT headers)
    install(FILES ${ZeroMQ_INCLUDE_DIR}/zmq.h DESTINATION include COMPONENT headers)
    install(FILES ${ZeroMQ_INCLUDE_DIR}/zmq_utils.h DESTINATION include COMPONENT headers)
    install(FILES ${cppzmq_INCLUDE_DIR}/zmq.hpp DESTINATION include COMPONENT headers)
    install(FILES ${cppzmq_INCLUDE_DIR}/zmq_addon.hpp DESTINATION include COMPONENT headers)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        install(FILES ${omniORB4_static_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)
        install(FILES ${omniORB4_thread_static_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)
        install(FILES ${omniORB4_COS4_static_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)
        install(FILES ${omniORB4_Dynamic4_static_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)
        install(FILES ${ZeroMQ_static_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)
        install(FILES ${omniORB4_LIBRARY_DEBUG} DESTINATION lib COMPONENT dynamic)
        install(FILES ${omniORB4_thread_LIBRARY_DEBUG} DESTINATION lib COMPONENT dynamic)
        install(FILES ${omniORB4_COS4_LIBRARY_DEBUG} DESTINATION lib COMPONENT dynamic)
        install(FILES ${omniORB4_Dynamic4_LIBRARY_DEBUG} DESTINATION lib COMPONENT dynamic)
        install(FILES ${ZeroMQ_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)
    else()
        install(FILES ${omniORB4_static_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
        install(FILES ${omniORB4_thread_static_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
        install(FILES ${omniORB4_COS4_static_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
        install(FILES ${omniORB4_Dynamic4_static_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
        install(FILES ${ZeroMQ_static_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
        install(FILES ${omniORB4_LIBRARY_RELEASE} DESTINATION lib COMPONENT dynamic)
        install(FILES ${omniORB4_thread_LIBRARY_RELEASE} DESTINATION lib COMPONENT dynamic)
        install(FILES ${omniORB4_COS4_LIBRARY_RELEASE} DESTINATION lib COMPONENT dynamic)
        install(FILES ${omniORB4_Dynamic4_LIBRARY_RELEASE} DESTINATION lib COMPONENT dynamic)
        install(FILES ${ZeroMQ_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
    endif()

    if(CMAKE_VS_PLATFORM_TOOLSET IN_LIST WINDOWS_SUPPORTED_VS_TOOLSETS)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            install(FILES ${omniORB4_RUNTIME_DEBUG} DESTINATION bin COMPONENT dynamic)
            install(FILES ${omniORB4_Dynamic4_RUNTIME_DEBUG} DESTINATION bin COMPONENT dynamic)
            install(FILES ${omniORB4_thread_RUNTIME_DEBUG} DESTINATION bin COMPONENT dynamic)
            install(FILES ${omniORB4_COS4_RUNTIME_DEBUG} DESTINATION bin COMPONENT dynamic)
            install(FILES ${ZeroMQ_RUNTIME_DEBUG} DESTINATION bin COMPONENT dynamic)
        else()
            install(FILES ${omniORB4_RUNTIME_RELEASE} DESTINATION bin COMPONENT dynamic)
            install(FILES ${omniORB4_Dynamic4_RUNTIME_RELEASE} DESTINATION bin COMPONENT dynamic)
            install(FILES ${omniORB4_thread_RUNTIME_RELEASE} DESTINATION bin COMPONENT dynamic)
            install(FILES ${omniORB4_COS4_RUNTIME_RELEASE} DESTINATION bin COMPONENT dynamic)
            install(FILES ${ZeroMQ_RUNTIME_RELEASE} DESTINATION bin COMPONENT dynamic)
        endif()
    endif()

    #pthreads
    if (TANGO_USE_PTHREAD)
        install(FILES ${pthread_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
        install(FILES ${pthread_static_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
        install(FILES ${pthread_RUNTIME_RELEASE} DESTINATION bin COMPONENT dynamic)
        install(FILES ${pthread_DBG_RELEASE} DESTINATION bin COMPONENT dynamic)
        install(FILES ${pthread_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)
        install(FILES ${pthread_static_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)
        install(FILES ${pthread_RUNTIME_DEBUG} DESTINATION bin COMPONENT dynamic)
        install(FILES ${pthread_DBG_DEBUG} DESTINATION bin COMPONENT dynamic)
        install(DIRECTORY ${pthread_INCLUDE_DIR}/ DESTINATION include)
    endif()

    #Jpeg
    if (TANGO_USE_JPEG)
        install(FILES ${JPEG_INCLUDE_DIRS}/jconfig.h DESTINATION include COMPONENT headers)
        install(FILES ${JPEG_INCLUDE_DIRS}/jmorecfg.h DESTINATION include COMPONENT headers)
        install(FILES ${JPEG_INCLUDE_DIRS}/jpeglib.h DESTINATION include COMPONENT headers)
        install(FILES ${JPEG_INCLUDE_DIRS}/jerror.h DESTINATION include COMPONENT headers)
        install(FILES ${JPEG_INCLUDE_DIRS}/turbojpeg.h DESTINATION include COMPONENT headers)
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            install(FILES ${JPEG_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)
            install(FILES ${TURBO_JPEG_LIBRARY_STATIC_DEBUG} DESTINATION lib COMPONENT static)
            install(FILES ${TURBO_JPEG_LIBRARY_DEBUG} DESTINATION lib COMPONENT static)

            install(FILES ${JPEG_RUNTIME_DEBUG} DESTINATION bin COMPONENT dynamic)
        else()
            install(FILES ${JPEG_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
            install(FILES ${TURBO_JPEG_LIBRARY_RELEASE} DESTINATION lib COMPONENT static)
            install(FILES ${TURBO_JPEG_LIBRARY_STATIC_RELEASE} DESTINATION lib COMPONENT static)

            install(FILES ${JPEG_RUNTIME_RELEASE} DESTINATION bin COMPONENT dynamic)
        endif()
    endif()

    if(TANGO_USE_TELEMETRY)
      if(BUILD_SHARED_LIBS)
        message(FATAL_ERROR "Missing installation code")
      else()
        if (NOT ZLIB_ROOT)
          get_filename_component(ZLIB_LIBRARY_DIR ${ZLIB_LIBRARY} DIRECTORY)
          get_filename_component(ZLIB_ROOT ${ZLIB_LIBRARY_DIR} DIRECTORY)
        endif()
        install(FILES ${ZLIB_ROOT}/include/zconf.h DESTINATION include COMPONENT headers)
        install(FILES ${ZLIB_ROOT}/include/zlib.h DESTINATION include COMPONENT headers)
        install(FILES ${ZLIB_ROOT}/include/zlib_name_mangling.h DESTINATION include COMPONENT headers)

        install(FILES ${ZLIB_ROOT}/bin/zlib1.dll DESTINATION bin)
        install(FILES ${ZLIB_ROOT}/lib/zlib.lib DESTINATION lib)

        if(NOT TANGO_OTEL_ROOT)
          message(FATAL_ERROR "Missing TANGO_OTEL_ROOT variable")
        endif()

        install(DIRECTORY ${TANGO_OTEL_ROOT}/lib/ DESTINATION lib)
        install(DIRECTORY ${TANGO_OTEL_ROOT}/cmake/ DESTINATION cmake)
        install(DIRECTORY ${TANGO_OTEL_ROOT}/share/cmake/ DESTINATION share/cmake)
        install(DIRECTORY ${TANGO_OTEL_ROOT}/share/pkgconfig/ DESTINATION share/pkgconfig)
      endif()
    endif()
endif()
