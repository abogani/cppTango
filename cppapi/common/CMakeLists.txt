set(SOURCES net.cpp)

add_library(common_objects OBJECT ${SOURCES})
add_dependencies(common_objects idl_objects)

if(WIN32)
    target_compile_definitions(common_objects PRIVATE "${windows_defs}")
else()
    target_compile_options(common_objects PRIVATE -fPIC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Do not define _TANGO_LIB when compiling Tango debug library on Linux
        # in order to keep the same behaviour as in the past:
        # A Linux Tango Device server using the debug tango lib will display
        # twice "Ready to accept requests" at startup
        target_compile_definitions(common_objects PRIVATE _REENTRANT)
    else()
        target_compile_definitions(common_objects PRIVATE _REENTRANT _TANGO_LIB)
    endif()
endif()

if(TANGO_USE_PCH)
    tango_target_pch(common_objects)
endif()
