include(${CMAKE_CURRENT_SOURCE_DIR}/config/config.cmake)

add_subdirectory(include/tango/common/log4tango)
add_subdirectory(src)

add_library(log4tango_objects OBJECT ${SOURCES})

if(WIN32)
    target_compile_definitions(log4tango_objects PRIVATE "${windows_defs}")
else()
    target_compile_options(log4tango_objects PRIVATE -fPIC)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
