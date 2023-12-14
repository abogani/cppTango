find_package(Catch2 3 REQUIRED)

add_executable(BddTests
    test_attr_manip.cpp
    utils/utils.cpp)

target_include_directories(BddTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BddTests PRIVATE tango Catch2::Catch2WithMain)
