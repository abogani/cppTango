find_package(Catch2 3 REQUIRED)

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
add_custom_target(log_dir ALL COMMAND ${CMAKE_COMMAND} -E make_directory ${TANGO_BDD_LOG_DIR})

add_library(bdd SHARED
    test_attr_manip.cpp
    utils/auto_device_class.cpp
    utils/bdd_server.cpp
    utils/entry_points.cpp
    utils/utils.cpp
    ${PLATFORM_IMPL})

target_link_libraries(bdd PUBLIC tango Catch2::Catch2)
target_include_directories(bdd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(BddServer
    main_server.cpp)

target_link_libraries(BddServer PRIVATE bdd)

add_executable(BddTests
    main_test.cpp)

target_link_libraries(BddTests PRIVATE bdd)

# TODO: configure_file instead?
target_compile_definitions(bdd PRIVATE
    "-DTANGO_TEST_BDD_SERVER_BINARY_PATH=\"$<TARGET_FILE:BddServer>\""
    "-DTANGO_TEST_BDD_OUTPUT_DIRECTORY_PATH=\"${TANGO_BDD_LOG_DIR}\"")
