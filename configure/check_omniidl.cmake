# Use a function to introduce a scope so that we can set CMAKE_CXX_FLAGS
function(test_omniidl)
    message(STATUS "Using OMNIIDL=${OMNIIDL}")
    message(STATUS "Testing omniidl for bug in generated c++ for IDL union")

    execute_process(
        COMMAND ${OMNIIDL} -bcxx -Wbh=.h -Wbs=SK.cpp -Wbd=DynSK.cpp -Wba
            ${CMAKE_CURRENT_SOURCE_DIR}/configure/test_omniidl.idl
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE OMNIIDL_RESULT
        OUTPUT_VARIABLE OMNIIDL_OUTPUT
        OUTPUT_VARIABLE OMNIIDL_ERROR
        )

    if (NOT OMNIIDL_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate c++ code with omniidl:\n${OMNIIDL_OUTPUT}\n${OMNIIDL_ERROR}")
    endif()

    if (WIN32)
        if(TARGET omniORB4::omniORB4-static)
          # Always statically link on Windows so we don't need the omniorb dll's to be in the PATH
          set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
          set(link_libs omniORB4::omniORB4-static;omniORB4::COS4-static;omniORB4::Dynamic4-static;ws2_32.lib)
        else()
          set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
          set(link_libs omniORB4::omniORB4;omniORB4::COS4;omniORB4::Dynamic4;ws2_32.lib)
        endif()

        set(defs "")
        foreach(def ${static_defs})
            list(APPEND defs "-D${def}")
        endforeach()
    else()
        set(link_libs omniORB4::omniORB4 omniORB4::COS4 omniORB4::Dynamic4)
        set(defs "")
    endif()

    try_run(TANGO_OMNIIDL_HAS_NO_UNION_BUG TANGO_OMNIIDL_CHECK_COMPILE
        ${CMAKE_CURRENT_BINARY_DIR}/
        SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/configure/test_omniidl.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/test_omniidlSK.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/test_omniidlDynSK.cpp
        CMAKE_FLAGS
            "-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_BINARY_DIR}"
        COMPILE_DEFINITIONS ${defs}
        LINK_LIBRARIES ${link_libs}
        COMPILE_OUTPUT_VARIABLE OMNIIDL_TEST_COMPILE_OUTPUT
        )

    if (NOT TANGO_OMNIIDL_CHECK_COMPILE)
        message(FATAL_ERROR "Failed to compile omniidl test program:\n${OMNIIDL_TEST_COMPILE_OUTPUT}")
    endif()

    if (NOT TANGO_OMNIIDL_HAS_NO_UNION_BUG EQUAL 0)
        message(FATAL_ERROR "${OMNIIDL} has bug in c++ code generation, will not use.")
    endif()
endfunction()

if (NOT DEFINED TANGO_OMNIIDL_HAS_NO_UNION_BUG)
    if (CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMILING_EMULATOR)
        message(STATUS "Cross compilation detected: skipping omniidl union bug check")
        set(TANGO_OMNIIDL_HAS_NO_UNION_BUG 0 CACHE INTERNAL "")
    else()
        test_omniidl()
    endif()
endif()
