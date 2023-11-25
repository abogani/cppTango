set(git_revision_cpp ${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp)
configure_file(git_revision.cpp.in ${git_revision_cpp})
set(SOURCES net.cpp utils.cpp assert.cpp telemetry.cpp thread_specific_storage.cpp ${git_revision_cpp})

add_library(common_objects OBJECT ${SOURCES})
add_dependencies(common_objects idl_objects)
set_cflags_and_include(common_objects)

if(WIN32)
    target_compile_definitions(common_objects PRIVATE "${windows_defs}")
else()
    target_compile_options(common_objects PRIVATE -fPIC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Do not define _TANGO_LIB when compiling Tango debug library on Linux
        # in order to keep the same behaviour as in the past:
        # A Linux Tango Device server using the debug tango lib will display
        # twice "Ready to accept requests" at startup
    else()
        target_compile_definitions(common_objects PRIVATE _TANGO_LIB)
    endif()
endif()

target_precompile_headers(common_objects PRIVATE ${BUILD_TANGO_LOCAL_INCLUDE_DIR}/tango/tango.h)
