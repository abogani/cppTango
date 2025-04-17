cmake_minimum_required(VERSION 3.18...4.0 FATAL_ERROR)
# minimum version: https://gitlab.com/tango-controls/docker/ci/cpptango/debian-minimum-versions
# maximum version: https://gitlab.com/tango-controls/docker/ci/cpptango/debian-maximum-cmake

project(cppTango LANGUAGES CXX
                 HOMEPAGE_URL "https://www.tango-controls.org"
                 DESCRIPTION "Distributed Control System")

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

set(CMAKE_TRY_COMPILE_CONFIGURATION "${CMAKE_BUILD_TYPE}")

option(TANGO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(BUILD_SHARED_LIBS "Build a shared library instead of static" ON)
option(TANGO_USE_LIBCPP "Build against libc++" OFF)

option(TANGO_ENABLE_COVERAGE "Instrument code for coverage analysis" OFF)
if (TANGO_ENABLE_COVERAGE AND NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
    message(WARNING "Code coverage is not supported for selected compiler.")
    set(TANGO_ENABLE_COVERAGE OFF)
endif()

set(TANGO_ENABLE_SANITIZER "" CACHE STRING "Enable sanitizer")
set(TANGO_SUPPORTED_SANITIZERS ASAN TSAN UBSAN MSAN)
set_property(CACHE TANGO_ENABLE_SANITIZER PROPERTY STRINGS ${TANGO_SUPPORTED_SANITIZERS})

if(TANGO_ENABLE_SANITIZER AND NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
    message(FATAL_ERROR "Sanitizers are not supported for selected compiler.")
endif()
if(TANGO_ENABLE_SANITIZER AND NOT (TANGO_ENABLE_SANITIZER IN_LIST TANGO_SUPPORTED_SANITIZERS))
    message(FATAL_ERROR "TANGO_ENABLE_SANITIZER must be one of: ${TANGO_SUPPORTED_SANITIZERS}.")
endif()
if(TANGO_ENABLE_SANITIZER STREQUAL "MSAN" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "MSAN is not supported for selected compiler.")
endif()
if(TANGO_ENABLE_SANITIZER)
    find_program(ADDR2LINE_PROGRAM "addr2line")
    find_program(LLVM_SYMBOLIZER_PROGRAM "llvm-symbolizer")
    if(NOT ADDR2LINE_PROGRAM)
        message(FATAL_ERROR "Could not find addr2line (required for sanitizers).")
    endif()
    if(NOT LLVM_SYMBOLIZER_PROGRAM AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Could not find llvm-symbolizer (required for sanitizers).")
    endif()
endif()


# telemetry
option(TANGO_USE_TELEMETRY "Enable telemetry (OpenTelemetry)" ON)
include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(TANGO_TELEMETRY_USE_HTTP "Enable http protocol for telemetry (OpenTelemetry)" ON "TANGO_USE_TELEMETRY" OFF)
CMAKE_DEPENDENT_OPTION(TANGO_TELEMETRY_USE_GRPC "Enable gRPC protocol for telemetry (OpenTelemetry)" ON "TANGO_USE_TELEMETRY" OFF)

#need to define the version of the library
set(MAJOR_VERSION "10")
set(MINOR_VERSION "1")
set(PATCH_VERSION "0")

if(WIN32)
    include(configure/cmake_win_defs.cmake)
endif()

include(configure/git.cmake)
git_describe(GIT_DESCRIPTION SUFFIX_VERSION)

#convenient versions
set(LIBRARY_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}${SUFFIX_VERSION}")
set(SO_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")

set(TANGO_GIT_REVISION "${GIT_DESCRIPTION}")

set(TANGO_HOST $ENV{TANGO_HOST})
include(configure/CMakeLists.txt)

set(TANGO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TANGO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

#source code
add_subdirectory("log4tango")
add_subdirectory("src")

if(BUILD_TESTING)
  add_subdirectory("tests")
endif()

add_library(tango $<TARGET_OBJECTS:log4tango_objects>
                  $<TARGET_OBJECTS:common_objects>
                  $<TARGET_OBJECTS:client_objects>
                  $<TARGET_OBJECTS:idl_objects>
                  $<TARGET_OBJECTS:server_objects>)

set_cflags_and_include(tango)

target_include_directories(tango INTERFACE
    $<BUILD_INTERFACE:${TANGO_SOURCE_DIR}/src/include>
    $<BUILD_INTERFACE:${TANGO_SOURCE_DIR}/log4tango/include>
    $<BUILD_INTERFACE:${TANGO_BINARY_DIR}/src/include>
    $<BUILD_INTERFACE:${TANGO_BINARY_DIR}/log4tango/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_features(tango INTERFACE cxx_std_17)

target_compile_definitions(tango INTERFACE "$<$<AND:$<BOOL:${BUILD_SHARED_LIBS}>,$<CXX_COMPILER_ID:MSVC>>:TANGO_HAS_DLL>")

add_library(Tango::Tango ALIAS tango)

if(WIN32)
    include(configure/cmake_win.cmake)
    include(configure/cpack_win.cmake)
else()
    include(configure/cmake_linux.cmake)
endif()

include(configure/install.cmake)

include(CPack)
