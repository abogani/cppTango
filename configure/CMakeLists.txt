include(GNUInstallDirs)

option(TANGO_USE_JPEG "Build jpeg support" ON)

set(TANGO_FIND_MODULES_PATH TangoCMakeModules)
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    option(TANGO_GIT_SUBMODULE_INIT "Initialise git submodules at configure time" ON)
    if (TANGO_GIT_SUBMODULE_INIT AND NOT EXISTS
            "${CMAKE_CURRENT_SOURCE_DIR}/${TANGO_FIND_MODULES_PATH}/.git")
        message(STATUS "Initialising TangoCMakeModules")
        set(git_command ${GIT_EXECUTABLE} submodule update --init TangoCMakeModules)
        execute_process(COMMAND ${git_command}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_INIT_RESULT)

        if (NOT GIT_SUBMOD_INIT_RESULT EQUAL "0")
            message(FATAL_ERROR "\"${git_command}\" failed with exit code ${GIT_SUBMOD_INIT_RESULT}")
        endif()
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${TANGO_FIND_MODULES_PATH})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(WIN32)
  set(WINDOWS_SUPPORTED_VS_TOOLSETS v141 v142 v143)
endif()

#assuming build platform == target
set(PLATFORM 32)
if(WIN32)
    if(CMAKE_CL_64)
        set(PLATFORM 64)
    endif()
else()
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(PLATFORM 64)
    endif()
endif()

message(STATUS "CMake: version ${CMAKE_VERSION}")
message(STATUS "Target platform: ${CMAKE_SYSTEM_NAME} ${PLATFORM}-bit")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} with version ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "cppTango: version ${TANGO_GIT_REVISION}")

set(IDL_VERSION_REQUESTED 6.0.2)
if(NOT tangoidl_FOUND)
  # Intentional check if we found it first so that cmake super projects like the TangoSourceDistribution
  # can fake the result. Once we require at least cmake 3.24 we can use the approach outlined at
  # https://cmake.org/cmake/help/latest/module/FetchContent.html#integrating-with-find-package
  find_package(tangoidl ${IDL_VERSION_REQUESTED} CONFIG REQUIRED)
endif()

set(IDL_VERSION_FOUND ${tangoidl_VERSION})
message(STATUS "IDL: version ${IDL_VERSION_FOUND}")

find_package(Threads REQUIRED)
find_package(cppzmq REQUIRED)
find_package(omniORB4 REQUIRED)

if (TANGO_USE_TELEMETRY)
    # workaround gRPC cmake bug fixed in
    # 5c9e4050 ([Build] fix `gRPCConfig.cmake` (#33361), 2023-09-06)
    include(CMakeFindDependencyMacro)

    find_package(gRPC REQUIRED)
    message(STATUS "Tango telemetry (OpenTelemetry) enabled")
    find_package(opentelemetry-cpp REQUIRED)

    if(TANGO_TELEMETRY_USE_GRPC AND NOT TARGET opentelemetry-cpp::otlp_grpc_exporter)
        message(FATAL_ERROR "The opentelemetry-cpp library was compiled without grpc support but TANGO_TELEMETRY_USE_GRPC is enabled.")
    elseif(TANGO_TELEMETRY_USE_HTTP AND NOT TARGET opentelemetry-cpp::otlp_http_exporter)
      message(FATAL_ERROR "The opentelemetry-cpp library was compiled without http support but TANGO_TELEMETRY_USE_HTTP is enabled.")
    endif()

    message(STATUS "opentelemetry-cpp: version ${OPENTELEMETRY_VERSION}")

    if(WIN32)
      find_package(CURL REQUIRED)
      find_package(nlohmann_json REQUIRED)

      find_package(ZLIB REQUIRED)
      # create variables pointing to the runtime DLL for later usage
      if(ZLIB_LIBRARY_RELEASE AND NOT ZLIB_RUNTIME_RELEASE)
        string(REGEX REPLACE "zlib.lib$" "zlib1.dll" ZLIB_RUNTIME_RELEASE "${ZLIB_LIBRARY_RELEASE}")
        set(ZLIB_RUNTIME_RELEASE "${ZLIB_RUNTIME_RELEASE}" CACHE FILEPATH "")
      endif()

      if(ZLIB_LIBRARY_DEBUG AND NOT ZLIB_RUNTIME_DEBUG)
        string(REGEX REPLACE "zlib.lib$" "zlib1.dll" ZLIB_RUNTIME_DEBUG "${ZLIB_LIBRARY_DEBUG}")
        set(ZLIB_RUNTIME_DEBUG "${ZLIB_RUNTIME_DEBUG}" CACHE FILEPATH "")
      endif()
    endif()

    if(TANGO_TELEMETRY_USE_GRPC)
      if(NOT DEFINED TANGO_TELEMETRY_EXPORTER_OPTION_NEW)
        try_compile(TANGO_TELEMETRY_EXPORTER_OPTION_NEW ${CMAKE_CURRENT_BINARY_DIR}/test_opentelemetry_exporter_option_new_name
                    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/configure/test_opentelemetry_exporter_option_new_name.cpp
                    LINK_LIBRARIES opentelemetry-cpp::otlp_grpc_log_record_exporter)
      endif()

      message(STATUS "opentelemetry-cpp uses new exporter name: ${TANGO_TELEMETRY_EXPORTER_OPTION_NEW}")
    endif()

    if(NOT TANGO_TELEMETRY_USE_GRPC AND NOT TANGO_TELEMETRY_USE_HTTP)
      message(FATAL_ERROR "At least one of TANGO_TELEMETRY_USE_GRPC/TANGO_TELEMETRY_USE_HTTP needs to be enabled")
    endif()
else()
    message(STATUS "Tango telemetry (OpenTelemetry) disabled")
endif()

if(NOT OMNIIDL)
    message(FATAL_ERROR "Could not find a usable omniidl")
endif()

function(tango_find_omniorb_version)
    if (DEFINED OMNIORB_PKG_VERSION)
        set(version ${OMNIORB_PKG_VERSION})
    elseif(WIN32)
        # There is no acconfig.h on Windows
        return()
    else()
        find_file(acconfig
            NAMES omniORB4/acconfig.h
            PATHS ${omniORB4_INCLUDE_DIR}
            NO_DEFAULT_PATH
            )

        if (NOT acconfig)
            message(WARNING "Could not find omniORB4/acconfig.h")
            set(OMNIORB_VERSION OMNIORB_VERSION-NOTFOUND PARENT_SCOPE)
            return()
        endif()

        file(STRINGS ${acconfig} version_info
            REGEX "^#define[ \t]+(OMNI_)?PACKAGE_VERSION.*$")
        list(LENGTH version_info version_info_length)

        if (NOT version_info_length EQUAL 1)
            message(WARNING "Could not find version information in ${acconfig}")
            set(OMNIORB_VERSION OMNIORB_VERSION-NOTFOUND PARENT_SCOPE)
            return()
        endif()

        string(REGEX REPLACE "^#define[ \t]+(OMNI_)?PACKAGE_VERSION[ \t]+\"(([0-9]+\\.?)+)\"" "\\2" version ${version_info})
    endif()

    set(OMNIORB_VERSION ${version} CACHE STRING "omniORB version")
    mark_as_advanced(OMNIORB_VERSION)
endfunction()

if (NOT DEFINED OMNIORB_VERSION)
    tango_find_omniorb_version()
endif()

# These define a semi-open "[min, max)" range of allowed omniORB versions
set(OMNIORB_VERSION_MINIMUM 4.3.0)
set(OMNIORB_VERSION_MAXIMUM 4.4.0)
if (CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(OMNIORB_VERSION_MINIMUM 4.3.1)
endif()

set(TANGO_SKIP_OMNIORB_VERSION_CHECK OFF CACHE BOOL "If set, skip the omniORB version check")
mark_as_advanced(TANGO_SKIP_OMNIORB_VERSION_CHECK)
if (NOT TANGO_SKIP_OMNIORB_VERSION_CHECK)
    if (OMNIORB_VERSION)
        if (OMNIORB_VERSION VERSION_LESS OMNIORB_VERSION_MINIMUM OR
            OMNIORB_VERSION VERSION_GREATER_EQUAL OMNIORB_VERSION_MAXIMUM)
            message(FATAL_ERROR
                "Found omniORB version (${OMNIORB_VERSION}) outside of supported range [${OMNIORB_VERSION_MINIMUM}, ${OMNIORB_VERSION_MAXIMUM}).\n"
                "Set -DTANGO_SKIP_OMNIORB_VERSION_CHECK=ON to disable this check (at your own risk).")
        endif()
        message(STATUS "Found OMNIORB_VERSION=${OMNIORB_VERSION}")
    elseif(NOT WIN32)
        message(WARNING "omniORB version not found")
    endif()
endif()

include(configure/check_omniidl.cmake)

# Check for jpeg support
if(TANGO_USE_JPEG)
    find_package(JPEG REQUIRED)
    set(JPEG_LIB libjpeg)
    set(JPEG_LIB_FLAG -ljpeg)
endif()

set(CPPZMQ_TRY_COMPILE_OUTPUT "" CACHE STRING "Compilation test for compatible cppzmq version")
mark_as_advanced(CPPZMQ_TRY_COMPILE_OUTPUT)

if(NOT DEFINED TANGO_CPPZMQ_USABLE_VERSION)
  try_compile(TANGO_CPPZMQ_USABLE_VERSION ${CMAKE_CURRENT_BINARY_DIR}/test_cppzmq_features
              SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/configure/test_cppzmq_features.cpp
              COMPILE_DEFINITIONS "-I ${cppzmq_INCLUDE_DIR} -I ${ZeroMQ_INCLUDE_DIR}"
              LINK_LIBRARIES cppzmq::cppzmq
              OUTPUT_VARIABLE CPPZMQ_TRY_COMPILE_OUTPUT)
endif()

set(msg "Check if cppzmq is present and recent enough: ${TANGO_CPPZMQ_USABLE_VERSION}")

if(${TANGO_CPPZMQ_USABLE_VERSION})
  message(STATUS ${msg})
else()
  message(FATAL_ERROR ${msg} "\n\nBuild output:\n" ${CPPZMQ_TRY_COMPILE_OUTPUT})
endif()

set(ZMQ_MIN_VER_MAJOR 4)
set(ZMQ_MIN_VER_MINOR 0)
set(ZMQ_MIN_VER_PATCH 5)

set(ZMQ_TRY_COMPILE_OUTPUT "" CACHE STRING "Compilation test for compatible libzmq version")
mark_as_advanced(ZMQ_TRY_COMPILE_OUTPUT)

if(NOT DEFINED TANGO_ZMQ_USABLE_VERSION)
  try_compile(TANGO_ZMQ_USABLE_VERSION ${CMAKE_CURRENT_BINARY_DIR}/test_zmq_version
              SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/configure/test_zmq_version.cpp
              COMPILE_DEFINITIONS " -DMINIMUM_VERSION_MAJOR=${ZMQ_MIN_VER_MAJOR}
              -DMINIMUM_VERSION_MINOR=${ZMQ_MIN_VER_MINOR}
              -DMINIMUM_VERSION_PATCH=${ZMQ_MIN_VER_PATCH}"
              LINK_LIBRARIES cppzmq::cppzmq
              OUTPUT_VARIABLE ZMQ_TRY_COMPILE_OUTPUT)
endif()

set(msg "Check if libzmq version is >= ${ZMQ_MIN_VER_MAJOR}.${ZMQ_MIN_VER_MINOR}.${ZMQ_MIN_VER_PATCH}: ${TANGO_ZMQ_USABLE_VERSION}")

if(${TANGO_ZMQ_USABLE_VERSION})
  message(STATUS ${msg})
else()
    message(FATAL_ERROR ${msg} "\n\nBuild output:\n" ${ZMQ_TRY_COMPILE_OUTPUT})
endif()

if(NOT WIN32)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      if(CMAKE_BUILD_TYPE MATCHES "(Release|RelWithDebInfo|MinSizeRel)")
            add_definitions(-D_FORTIFY_SOURCE=2)
            add_compile_options(-fstack-protector-strong)
      elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
          add_compile_options(-Wall -Wextra -Wformat -Werror=format-security -pedantic -Og -g)
      else()
          message(FATAL_ERROR "Invalid CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
      endif()

        if(TANGO_ENABLE_SANITIZER STREQUAL "ASAN")
            add_compile_options(-fsanitize=address -fsanitize=leak
                -fno-omit-frame-pointer
                -Og -g)
            add_link_options(-fsanitize=address -fsanitize=leak)
        elseif(TANGO_ENABLE_SANITIZER STREQUAL "TSAN")
            add_compile_options(-fsanitize=thread -Og -g)
            add_link_options(-fsanitize=thread)
        elseif(TANGO_ENABLE_SANITIZER STREQUAL "UBSAN")
            add_compile_options(-fsanitize=undefined
                -fno-omit-frame-pointer
                -Og -g)
            add_link_options(-fsanitize=undefined)
            if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                add_compile_options(
                    -fsanitize=bool
                    -fsanitize=implicit-integer-truncation
                    -fsanitize=implicit-integer-arithmetic-value-change
                    -fsanitize=implicit-conversion
                    -fsanitize=integer
                    -fsanitize=nullability)
            endif()
        elseif(TANGO_ENABLE_SANITIZER STREQUAL "MSAN")
            add_compile_options(-fsanitize=memory
                -fno-omit-frame-pointer
                -fsanitize-memory-track-origins
                -Og -g)
            add_link_options(-fsanitize=memory)
        endif()
    endif()

    if(TANGO_USE_LIBCPP)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-stdlib=libc++)
            add_link_options(-stdlib=libc++)
            add_link_options(-fuse-ld=lld)
            if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
                # This is unsupported by Apple's clang linker.
                add_link_options(LINKER:--allow-shlib-undefined)
            else()
                # It needs to be --undefined dynamic_lookup instead.
                add_link_options(-undefined dynamic_lookup)
            endif()
        else()
            message(WARNING "TANGO_USE_LIBCPP=ON is only supported with clang.")
        endif()
    endif()
endif()

if (TANGO_ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
endif()

if(TANGO_WARNINGS_AS_ERRORS)
  if(WIN32)
    add_compile_options(/WX)
    # LNK4099: PDB was not found
    add_link_options(/WX /ignore:4099)
  else()
    add_compile_options(-Werror)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wdeprecated -Wdeprecated-implementations -Wextra-semi)
    endif()
  endif()
endif()

include(configure/functions.cmake)

include(GNUInstallDirs)
