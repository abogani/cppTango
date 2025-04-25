cmake_minimum_required(VERSION 3.18...4.0 FATAL_ERROR)
# minimum version: https://gitlab.com/tango-controls/docker/ci/cpptango/debian-minimum-versions
# maximum version: https://gitlab.com/tango-controls/docker/ci/cpptango/debian-maximum-cmake

project(dummy LANGUAGES CXX)

find_package(Tango CONFIG REQUIRED)

add_executable(dummy dummy.cpp)

target_link_libraries(dummy PUBLIC Tango::Tango)
