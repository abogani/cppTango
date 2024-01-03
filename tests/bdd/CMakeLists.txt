find_package(Catch2 3 REQUIRED)

add_library(bdd SHARED
    test_attr_manip.cpp
    utils/auto_device_class.cpp
    utils/entry_points.cpp
    utils/utils.cpp)

target_link_libraries(bdd PUBLIC tango Catch2::Catch2)
target_include_directories(bdd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(BddServer
    main_server.cpp)

target_link_libraries(BddServer PRIVATE bdd)

add_executable(BddTests
    main_test.cpp)

target_link_libraries(BddTests PRIVATE bdd)
