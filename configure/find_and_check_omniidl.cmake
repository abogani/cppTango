if (NOT WIN32 AND NOT DEFINED OMNIORB_PKG_LIBRARIES)
    return()
endif()

# Use a function to introduce a scope so that we can set CMAKE_CXX_FLAGS
function(test_omniidl)
    message(STATUS "Testing omniidl for bug in generated c++ for IDL union")
    execute_process(
        COMMAND ${OMNIIDL} -bcxx -Wbh=.h -Wbs=SK.cpp -Wbd=DynSK.cpp -Wba
            ${CMAKE_SOURCE_DIR}/configure/test_omniidl.idl
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE OMNIIDL_RESULT
        OUTPUT_VARIABLE OMNIIDL_OUTPUT
        OUTPUT_VARIABLE OMNIIDL_ERROR
        )

    if (NOT OMNIIDL_RESULT EQUAL 0)
        message(WARNING "Failed to generate c++ code with omniidl:\n${OMNIIDL_OUTPUT}\n${OMNIIDL_ERROR}")
        set(OMNIIDL FALSE)
        return()
    endif()

    if (WIN32)
        # Always statically link on Windows so we don't need the omniorb dll's to be in the PATH
        set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
        set(link_libs ${OMNIORB_PKG_LIBRARIES_STA_DEBUG};ws2_32.lib)
        set(defs "-D__x86__" "-D__NT__" "-D__OSVERSION__=4" "-D__WIN32__" "-D_WIN32_WINNT=0x0400")
        foreach(def ${static_defs})
            list(APPEND defs "-D${def}")
        endforeach()
    else()
        set(link_libs ${OMNIORB_PKG_LIBRARIES} ${OMNICOS_PKG_LIBRARIES} ${OMNIDYN_PKG_LIBRARIES})
        set(defs "")
    endif()

    set(inc_dirs
        ${CMAKE_CURRENT_BINARY_DIR}
        ${OMNIORB_PKG_INCLUDE_DIRS}
        ${OMNIDYN_PKG_INCLUDE_DIRS}
        ${OMNICOS_PKG_INCLUDE_DIRS})

    set(link_dirs
        ${OMNIORB_PKG_LIBRARY_DIRS}
        ${OMNIDYN_PKG_LIBRARY_DIRS}
        ${OMNICOS_PKG_LIBRARY_DIRS})

    try_run(TANGO_OMNIIDL_HAS_NO_UNION_BUG TANGO_OMNIIDL_CHECK_COMPILE
        ${CMAKE_CURRENT_BINARY_DIR}/
        SOURCES
            ${CMAKE_SOURCE_DIR}/configure/test_omniidl.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/test_omniidlSK.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/test_omniidlDynSK.cpp
        CMAKE_FLAGS
            "-DINCLUDE_DIRECTORIES=${inc_dirs}"
            "-DLINK_DIRECTORIES=${link_dirs}"
        COMPILE_DEFINITIONS ${defs}
        LINK_LIBRARIES ${link_libs}
        COMPILE_OUTPUT_VARIABLE OMNIIDL_TEST_COMPILE_OUTPUT
        )

    if (NOT TANGO_OMNIIDL_CHECK_COMPILE)
        message(WARNING "Failed to compile omniidl test program:\n${OMNIIDL_TEST_COMPILE_OUTPUT}")
        set(OMNIIDL FALSE)
        return()
    endif()

    if (NOT TANGO_OMNIIDL_HAS_NO_UNION_BUG EQUAL 0)
        message(WARNING "${OMNIIDL} has bug in c++ code generation, will not use.")
        set(OMNIIDL FALSE)
    endif()
endfunction()

if (NOT DEFINED TANGO_OMNIIDL_HAS_NO_UNION_BUG)
    test_omniidl()
endif()
