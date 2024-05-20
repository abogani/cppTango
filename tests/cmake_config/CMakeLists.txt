cmake_minimum_required(VERSION 3.18...3.28 FATAL_ERROR)
# minimum version: https://gitlab.com/tango-controls/docker/ci/cpptango/debian-minimum-versions
# maximum version: https://gitlab.com/tango-controls/docker/ci/cpptango/debian-maximum-cmake

project(dummy LANGUAGES CXX)

find_package(Tango REQUIRED)

add_executable(dummy dummy.cpp)

target_link_libraries(dummy PUBLIC Tango::Tango)
