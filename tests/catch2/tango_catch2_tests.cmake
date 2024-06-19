find_package(Catch2 3.3.0 REQUIRED)
include(Catch)

set(TANGO_CATCH2_TESTS_DIR ${CMAKE_CURRENT_LIST_DIR})

function(tango_catch2_tests_create)
    set(TEST_FILES ${ARGN})

    set(PLATFORM_IMPL "${TANGO_CATCH2_TESTS_DIR}/utils/platform/ready_string_finder.cpp")
    if(WIN32)
        list(APPEND PLATFORM_IMPL ${TANGO_CATCH2_TESTS_DIR}/utils/platform/impl_win32.cpp)
    elseif(UNIX)
        list(APPEND PLATFORM_IMPL ${TANGO_CATCH2_TESTS_DIR}/utils/platform/impl_unix.cpp)
        if(APPLE)
            list(APPEND PLATFORM_IMPL ${TANGO_CATCH2_TESTS_DIR}/utils/platform/unix/impl_macos.cpp)
        else()
            list(APPEND PLATFORM_IMPL ${TANGO_CATCH2_TESTS_DIR}/utils/platform/unix/impl_linux.cpp)
        endif()
    else()
        message(FATAL_ERROR "Unsupported platform for Catch2 tests")
    endif()

    set(TANGO_CATCH2_LOG_DIR ${CMAKE_CURRENT_BINARY_DIR}/catch2_test_logs)
    set(TANGO_CATCH2_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/catch2_server_output)
    set(TANGO_CATCH2_FILEDB_DIR ${CMAKE_CURRENT_BINARY_DIR}/catch2_test_filedb)

    add_custom_target(Catch2ServerLogs ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TANGO_CATCH2_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TANGO_CATCH2_LOG_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TANGO_CATCH2_FILEDB_DIR}"
        )

    add_test(NAME catch2::setup COMMAND
        ${CMAKE_COMMAND}
        "-DTANGO_CATCH2_LOG_DIR=${TANGO_CATCH2_OUTPUT_DIR}"
        -P "${TANGO_CATCH2_TESTS_DIR}/clean_log_dir.cmake")
    add_test(NAME catch2::cleanup COMMAND
        ${CMAKE_COMMAND}
        "-DTANGO_CATCH2_LOG_DIR=${TANGO_CATCH2_OUTPUT_DIR}"
        -P "${TANGO_CATCH2_TESTS_DIR}/check_for_server_logs.cmake")

    set_tests_properties(catch2::setup PROPERTIES FIXTURES_SETUP CATCH2)
    set_tests_properties(catch2::cleanup PROPERTIES FIXTURES_CLEANUP CATCH2)

    add_executable(Catch2Tests
        ${TEST_FILES}
        ${TANGO_CATCH2_TESTS_DIR}/test_test_server.cpp
        ${TANGO_CATCH2_TESTS_DIR}/test_auto_command.cpp
        ${TANGO_CATCH2_TESTS_DIR}/test_auto_attr.cpp
        ${TANGO_CATCH2_TESTS_DIR}/utils/auto_device_class.cpp
        ${TANGO_CATCH2_TESTS_DIR}/utils/test_server.cpp
        ${TANGO_CATCH2_TESTS_DIR}/utils/entry_points.cpp
        ${TANGO_CATCH2_TESTS_DIR}/utils/utils.cpp
        common.cpp
        ${PLATFORM_IMPL})

    target_link_libraries(Catch2Tests PUBLIC Tango::Tango Catch2::Catch2 Threads::Threads)
    target_link_libraries(Catch2Tests PRIVATE $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)
    target_include_directories(Catch2Tests PUBLIC ${TANGO_CATCH2_TESTS_DIR})

    if (WIN32)
        # On Windows, we need to copy any dependent DLLs into the test directory
        # so that we can run the Catch2Tests EXE.
        #
        # When we move to CMake 3.22 (minimum) we can pass DL_PATHS to
        # catch_discover_tests to avoid this copying.

        # TODO: Use -E copy -t when on CMake 3.26
        add_custom_command(TARGET Catch2Tests POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E "$<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:Catch2Tests>>,copy;$<TARGET_RUNTIME_DLLS:Catch2Tests>;$<TARGET_FILE_DIR:Catch2Tests>,true>"
          COMMAND_EXPAND_LISTS)

        # The JPEG::JPEG target is an IMPORTED UNKNOWN library, which means it will not be found by TARGET_RUNTIME_DLLS
        if (TANGO_USE_JPEG)
            if(JPEG_RUNTIME_RELEASE)
                add_custom_command(TARGET Catch2Tests POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy ${JPEG_RUNTIME_RELEASE} $<TARGET_FILE_DIR:Catch2Tests>
                  COMMAND_EXPAND_LISTS)
            endif()
            if(JPEG_RUNTIME_DEBUG)
                add_custom_command(TARGET Catch2Tests POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy ${JPEG_RUNTIME_DEBUG} $<TARGET_FILE_DIR:Catch2Tests>
                  COMMAND_EXPAND_LISTS)
            endif()
        endif()

        # copy zlib dll manually
        if (TANGO_USE_TELEMETRY)
            if (ZLIB_RUNTIME_RELEASE AND ZLIB_RUNTIME_DEBUG)
                add_custom_command(TARGET Catch2Tests POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy $<$<CONFIG:Debug>:${ZLIB_RUNTIME_DEBUG}:${ZLIB_RUNTIME_RELEASE}> $<TARGET_FILE_DIR:Catch2Tests>
                  COMMAND_EXPAND_LISTS)
            elseif(ZLIB_RUNTIME_RELEASE)
                add_custom_command(TARGET Catch2Tests POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy ${ZLIB_RUNTIME_RELEASE} $<TARGET_FILE_DIR:Catch2Tests>
                  COMMAND_EXPAND_LISTS)
            elseif(ZLIB_RUNTIME_DEBUG)
                add_custom_command(TARGET Catch2Tests POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy ${ZLIB_RUNTIME_DEBUG} $<TARGET_FILE_DIR:Catch2Tests>
                  COMMAND_EXPAND_LISTS)
            endif()
        endif()

        set(SERVER_NAME "TestServer.exe")
        # By default on Windows, administrator privileges are required to create symlinks so it
        # is easiest to avoid them and just make a copy here.
        add_custom_target(TestServer ALL
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Catch2Tests> ${SERVER_NAME}
            WORKING_DIRECTORY $<TARGET_FILE_DIR:Catch2Tests>
            )
        set(SERVER_PATH "$<TARGET_FILE_DIR:Catch2Tests>/${SERVER_NAME}")
    else()
        set(SERVER_NAME "TestServer")
        add_custom_target(TestServer ALL
            COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:Catch2Tests> ${SERVER_NAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
        set(SERVER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${SERVER_NAME}")
    endif()

    target_compile_definitions(Catch2Tests PRIVATE
        "-DTANGO_TEST_CATCH2_SERVER_BINARY_PATH=\"${SERVER_PATH}\""
        "-DTANGO_TEST_CATCH2_OUTPUT_DIRECTORY_PATH=\"${TANGO_CATCH2_OUTPUT_DIR}\""
        "-DTANGO_TEST_CATCH2_RESOURCE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
        "-DTANGO_TEST_CATCH2_LOG_DIRECTORY_PATH=\"${TANGO_CATCH2_LOG_DIR}\""
        "-DTANGO_TEST_CATCH2_TEST_BINARY_NAME=\"$<TARGET_FILE_NAME:Catch2Tests>\""
        "-DTANGO_TEST_CATCH2_SERVER_BINARY_NAME=\"${SERVER_NAME}\""
        "-DTANGO_TEST_CATCH2_FILEDB_DIRECTORY_PATH=\"${TANGO_CATCH2_FILEDB_DIR}\""
        ${COMMON_TEST_DEFS})

    catch_discover_tests(Catch2Tests TEST_PREFIX "catch2::"
        EXTRA_ARGS --warn NoAssertions --log-file-per-test-case
        PROPERTIES FIXTURES_REQUIRED CATCH2)

endfunction()
