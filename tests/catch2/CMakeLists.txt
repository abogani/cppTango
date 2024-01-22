find_package(Catch2 3 REQUIRED)
include(Catch)

set(PLATFORM_IMPL "")
if(WIN32)
    list(APPEND PLATFORM_IMPL utils/platform/impl_win32.cpp)
elseif(UNIX)
    list(APPEND PLATFORM_IMPL utils/platform/impl_unix.cpp)
    if(APPLE)
        list(APPEND PLATFORM_IMPL utils/platform/unix/impl_macos.cpp)
    else()
        list(APPEND PLATFORM_IMPL utils/platform/unix/impl_linux.cpp)
    endif()
else()
    message(FATAL_ERROR "Unsupported platform for Catch2 tests")
endif()

set(TANGO_CATCH2_LOG_DIR ${CMAKE_CURRENT_BINARY_DIR}/server_logs)

add_test(NAME catch2::setup COMMAND
    ${CMAKE_COMMAND}
    "-DTANGO_CATCH2_LOG_DIR=${TANGO_CATCH2_LOG_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/make_log_dir.cmake")
add_test(NAME catch2::cleanup COMMAND
    ${CMAKE_COMMAND}
    "-DTANGO_CATCH2_LOG_DIR=${TANGO_CATCH2_LOG_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_for_server_logs.cmake")

set_tests_properties(catch2::setup PROPERTIES FIXTURES_SETUP CATCH2)
set_tests_properties(catch2::cleanup PROPERTIES FIXTURES_CLEANUP CATCH2)

add_executable(Catch2Tests
    test_attr_manip.cpp
    test_test_server.cpp
    test_auto_command.cpp
    test_auto_attr.cpp
    utils/auto_device_class.cpp
    utils/test_server.cpp
    utils/entry_points.cpp
    utils/utils.cpp
    ${PLATFORM_IMPL})

target_link_libraries(Catch2Tests PUBLIC tango Catch2::Catch2)
target_include_directories(Catch2Tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(TestServer ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:Catch2Tests> TestServer
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

# TODO: configure_file instead?
target_compile_definitions(Catch2Tests PRIVATE
    "-DTANGO_TEST_CATCH2_SERVER_BINARY_PATH=\"${CMAKE_CURRENT_BINARY_DIR}/TestServer\""
    "-DTANGO_TEST_CATCH2_OUTPUT_DIRECTORY_PATH=\"${TANGO_CATCH2_LOG_DIR}\"")

catch_discover_tests(Catch2Tests TEST_PREFIX "catch2::" EXTRA_ARGS --warn NoAssertions PROPERTIES FIXTURES_REQUIRED CATCH2)
