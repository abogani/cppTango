find_package(Catch2 3.1.1 REQUIRED)
include(Catch)

set(TANGO_CATCH2_TESTS_DIR ${CMAKE_CURRENT_LIST_DIR})

function(tango_catch2_tests_create)
    set(TEST_FILES ${ARGN})

    set(PLATFORM_IMPL "")
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

    add_custom_target(Catch2ServerLogs ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TANGO_CATCH2_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TANGO_CATCH2_LOG_DIR}")

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
        ${PLATFORM_IMPL})

    target_link_libraries(Catch2Tests PUBLIC tango Catch2::Catch2 Threads::Threads)
    target_include_directories(Catch2Tests PUBLIC ${TANGO_CATCH2_TESTS_DIR})

    if (WIN32)
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
        "-DTANGO_TEST_CATCH2_SERVER_BINARY_PATH=\"${CMAKE_CURRENT_BINARY_DIR}/TestServer\""
        "-DTANGO_TEST_CATCH2_OUTPUT_DIRECTORY_PATH=\"${TANGO_CATCH2_OUTPUT_DIR}\""
        "-DTANGO_TEST_CATCH2_RESOURCE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
        "-DTANGO_TEST_CATCH2_LOG_DIRECTORY_PATH=\"${TANGO_CATCH2_LOG_DIR}\""
        "-DTANGO_TEST_CATCH2_TEST_BINARY_NAME=\"$<TARGET_FILE_NAME:Catch2Tests>\""
        "-DTANGO_TEST_CATCH2_SERVER_BINARY_NAME=\"${SERVER_NAME}\"")

    catch_discover_tests(Catch2Tests TEST_PREFIX "catch2::"
        EXTRA_ARGS --warn NoAssertions --log-file-per-test-case
        PROPERTIES FIXTURES_REQUIRED CATCH2)

endfunction()
