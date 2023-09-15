cmake_minimum_required(VERSION 3.18...3.26 FATAL_ERROR)
# minimum version: https://gitlab.com/tango-controls/docker/ci/cpptango/debian-minimum-versions
# maximum version: https://gitlab.com/tango-controls/docker/ci/cpptango/debian-maximum-cmake

project(cppTango LANGUAGES CXX
                 HOMEPAGE_URL "https://www.tango-controls.org"
                 DESCRIPTION "Distributed Control System")

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

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

#need to define the version of the library
set(MAJOR_VERSION "10")
set(MINOR_VERSION "0")
set(PATCH_VERSION "0")

if(WIN32)
    include(configure/cmake_win_defs.cmake)
endif()

include(configure/git.cmake)
find_package(Git QUIET)
if(EXISTS "${CMAKE_SOURCE_DIR}/.git" AND Git_FOUND)
  execute_process(
      COMMAND "${GIT_EXECUTABLE}" describe --tags --exclude=*test* --always HEAD
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_DESCRIBE_STDOUT
      ERROR_VARIABLE GIT_DESCRIBE_STDERR
      RESULT_VARIABLE GIT_DESCRIBE_RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(GIT_DESCRIBE_RESULT)
    message(WARNING "Could not fetch the git revision (error ${GIT_DESCRIBE_RESULT}): ${GIT_DESCRIBE_STDERR}")
    set(GIT_DESCRIBE_STDOUT "unknown")
    set(SUFFIX_VERSION "dev")
  endif()

  # determine if we are on a tag or not
  execute_process(
      COMMAND "${GIT_EXECUTABLE}" name-rev --name-only --tags --no-undefined HEAD
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_NAMEREV_STDOUT
      ERROR_VARIABLE GIT_NAMEREV_STDERR
      RESULT_VARIABLE GIT_NAMEREV_RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(GIT_NAMEREV_RESULT)
      set(SUFFIX_VERSION "dev")
    else()
      # tagged version, empty suffix
      set(SUFFIX_VERSION "")
    endif()
else()
    set(GIT_DESCRIBE_STDOUT "unknown")
    set(SUFFIX_VERSION "")
endif()

#convenient versions
set(LIBRARY_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}${SUFFIX_VERSION}")
set(SO_VERSION "${MAJOR_VERSION}")

set(TANGO_GIT_REVISION "${GIT_DESCRIBE_STDOUT}")

set(TANGO_HOST $ENV{TANGO_HOST})
include(configure/CMakeLists.txt)

SET(BUILD_TANGO_LOCAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
message(STATUS "Using ${BUILD_TANGO_LOCAL_INCLUDE_DIR} as build-time include directory")
include_directories(${BUILD_TANGO_LOCAL_INCLUDE_DIR})
include_directories(log4tango/include)
#required for generated include/tango/idl/tango.h, include/tango/common/tango_const.h and config.h
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/log4tango/include)

#source code
add_subdirectory("log4tango")
add_subdirectory("src")

if(BUILD_TESTING)

  find_program(DOCKER_BINARY docker)
  if(NOT DOCKER_BINARY)
      message(WARNING "The tests can not be run as docker is missing.")
  endif()

  if(BUILD_SHARED_LIBS)
    add_subdirectory("tests")
  else()
    message(WARNING "Not building the tests, because that is currently supported only when BUILD_SHARED_LIBS is ON")
    set(BUILD_TESTING OFF)
  endif()
endif()


add_library(tango $<TARGET_OBJECTS:log4tango_objects>
                  $<TARGET_OBJECTS:common_objects>
                  $<TARGET_OBJECTS:client_objects>
                  $<TARGET_OBJECTS:idl_objects>
                  $<TARGET_OBJECTS:server_objects>)

target_link_libraries(tango
    PUBLIC
        ${CMAKE_DL_LIBS}
        omniORB4::omniORB4
        omniORB4::COS4
        omniORB4::Dynamic4
        cppzmq::cppzmq
    )

target_include_directories(tango PUBLIC
    ${cppzmq_INCLUDE_DIR}
    ${omniORB4_INCLUDE_DIR}
)

if(TANGO_USE_JPEG)
    target_link_libraries(tango PUBLIC JPEG::JPEG)
endif()

if(WIN32)
    include(configure/cmake_win.cmake)
    include(configure/cpack_win.cmake)
else()
    include(configure/cmake_linux.cmake)
endif()

include(CPack)
