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
    message(FATAL_ERROR "Unsupported platform for BDD tests")
endif()

set(TANGO_BDD_LOG_DIR ${CMAKE_CURRENT_BINARY_DIR}/server_logs)

add_test(NAME bdd::setup COMMAND
    ${CMAKE_COMMAND}
    "-DTANGO_BDD_LOG_DIR=${TANGO_BDD_LOG_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/make_log_dir.cmake")
add_test(NAME bdd::cleanup COMMAND
    ${CMAKE_COMMAND}
    "-DTANGO_BDD_LOG_DIR=${TANGO_BDD_LOG_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_for_server_logs.cmake")

set_tests_properties(bdd::setup PROPERTIES FIXTURES_SETUP BDD)
set_tests_properties(bdd::cleanup PROPERTIES FIXTURES_CLEANUP BDD)

add_executable(BddTests
    test_attr_manip.cpp
    test_bdd_server.cpp
    utils/auto_device_class.cpp
    utils/bdd_server.cpp
    utils/entry_points.cpp
    utils/utils.cpp
    ${PLATFORM_IMPL})

target_link_libraries(BddTests PUBLIC tango Catch2::Catch2)
target_include_directories(BddTests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(BddServer ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:BddTests> BddServer
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

# TODO: configure_file instead?
target_compile_definitions(BddTests PRIVATE
    "-DTANGO_TEST_BDD_SERVER_BINARY_PATH=\"${CMAKE_CURRENT_BINARY_DIR}/BddServer\""
    "-DTANGO_TEST_BDD_OUTPUT_DIRECTORY_PATH=\"${TANGO_BDD_LOG_DIR}\"")

catch_discover_tests(BddTests TEST_PREFIX "bdd::" EXTRA_ARGS --warn NoAssertions PROPERTIES FIXTURES_REQUIRED BDD)
