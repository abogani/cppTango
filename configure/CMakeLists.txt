include(FindPkgConfig)
include(GNUInstallDirs)

option(TANGO_USE_JPEG "Build jpeg support" ON)

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
message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID} with version ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Environment: TANGO_HOST=$ENV{TANGO_HOST}")

if(TANGO_CPPZMQ_BASE)
    message("Using TANGO_CPPZMQ_BASE=${TANGO_CPPZMQ_BASE}")
    include_directories(SYSTEM ${TANGO_CPPZMQ_BASE}/include)
endif()

if(TANGO_IDL_BASE)
    message("Using TANGO_IDL_BASE=${TANGO_IDL_BASE}")
    set(IDL_PKG_INCLUDE_DIRS ${TANGO_IDL_BASE}/include)
else()
    pkg_search_module(IDL_PKG REQUIRED tangoidl)
endif()

if(TANGO_OMNI_BASE)
    message("Using TANGO_OMNI_BASE=${TANGO_OMNI_BASE}")
    set(OMNIORB_PKG_INCLUDE_DIRS ${TANGO_OMNI_BASE}/include)
    if(WIN32)
        set(OMNIIDL_PATH ${TANGO_OMNI_BASE}/bin/x86_win32/)
        link_directories(${TANGO_OMNI_BASE}/lib/x86_win32/)
        include_directories(SYSTEM ${TANGO_OMNI_BASE}/include)
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(OMNIORB_PKG_LIBRARIES_DYN "omniORB4_rtd.lib;omniDynamic4_rtd.lib;omnithread_rtd.lib;COS4_rtd.lib")
            set(OMNIORB_PKG_LIBRARIES_STA "omniORB4d.lib;omniDynamic4d.lib;omnithreadd.lib;COS4d.lib")
        else()
            set(OMNIORB_PKG_LIBRARIES_DYN "omniORB4_rt.lib;omniDynamic4_rt.lib;omnithread_rt.lib;COS4_rt.lib")
            set(OMNIORB_PKG_LIBRARIES_STA "omniORB4.lib;omniDynamic4.lib;omnithread.lib;COS4.lib")
        endif()
    else()
        set(OMNIIDL_PATH ${TANGO_OMNI_BASE}/bin/)
        include_directories(SYSTEM ${TANGO_OMNI_BASE}/include)
        link_directories(${TANGO_OMNI_BASE}/lib)
        set(OMNIORB_PKG_LIBRARIES "-lomniORB4 -lomnithread -lCOS4 -lomniDynamic4")
    endif()
else()
    #CORBA
    pkg_search_module(OMNIORB_PKG REQUIRED omniORB4)
    #COS
    pkg_search_module(OMNICOS_PKG REQUIRED omniCOS4)
    #dynamic CORBA
    pkg_search_module(OMNIDYN_PKG REQUIRED omniDynamic4)
endif()

if(TANGO_ZMQ_BASE)
    message("Using TANGO_ZMQ_BASE=${TANGO_ZMQ_BASE}")
    set(ZMQ_PKG_INCLUDE_DIRS ${TANGO_ZMQ_BASE}/include)
    if (WIN32)
        link_directories(${TANGO_ZMQ_BASE}/lib/Debug)
        link_directories(${TANGO_ZMQ_BASE}/lib/Release)
        link_directories(${TANGO_ZMQ_BASE}/bin/Debug)
        link_directories(${TANGO_ZMQ_BASE}/bin/Release)
        if(CMAKE_VS_PLATFORM_TOOLSET IN_LIST WINDOWS_SUPPORTED_VS_TOOLSETS)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(ZMQ_PKG_LIBRARIES_STA "libzmq-v141-mt-sgd-4_0_5.lib")
                set(ZMQ_PKG_LIBRARIES_DYN "libzmq-v141-mt-gd-4_0_5.lib")
            else()
                set(ZMQ_PKG_LIBRARIES_STA "libzmq-v141-mt-s-4_0_5.lib")
                set(ZMQ_PKG_LIBRARIES_DYN "libzmq-v141-mt-4_0_5.lib")
            endif()
        endif()
    else()
        set(ZMQ_PKG_LIBRARIES "-lzmq")
        link_directories(${TANGO_ZMQ_BASE}/lib)
    endif()
    include_directories(SYSTEM ${TANGO_ZMQ_BASE}/include)
else()
    pkg_search_module(ZMQ_PKG REQUIRED libzmq)
endif()

# Check for jpeg support
if(TANGO_USE_JPEG)
    if(TANGO_JPEG_BASE)
        message("Using TANGO_JPEG_BASE=${TANGO_JPEG_BASE}")
        include_directories(SYSTEM ${TANGO_JPEG_BASE}/include)
        link_directories(${TANGO_JPEG_BASE}/lib)
        if (WIN32)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(JPEG_PKG_LIBRARIES_STA "jpeg-static${JPEG_DEBUG_POSTFIX}.lib")
                set(JPEG_PKG_LIBRARIES_DYN "jpeg${JPEG_DEBUG_POSTFIX}.lib")
            else()
                set(JPEG_PKG_LIBRARIES_STA "jpeg-static.lib")
                set(JPEG_PKG_LIBRARIES_DYN "jpeg.lib")
            endif()
        else()
            set(JPEG_LIBRARIES "-ljpeg")
        endif()
    else()
        find_package(JPEG REQUIRED)
    endif()
endif()

if(PTHREAD_WIN)
    message("Using PTHREAD_WIN=${PTHREAD_WIN}")
    add_definitions(-DHAVE_PTHREAD_NP_H)
    set(PTHREAD_WIN_PKG_INCLUDE_DIRS ${PTHREAD_WIN}/include)
    link_directories(${PTHREAD_WIN}/lib)
    link_directories(${PTHREAD_WIN}/bin)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(PTHREAD_WIN_PKG_LIBRARIES_DYN "pthreadVC2d.lib")
        set(PTHREAD_WIN_PKG_LIBRARIES_STA "pthreadVC2-sd.lib")
    else()
        set(PTHREAD_WIN_PKG_LIBRARIES_DYN "pthreadVC2.lib")
        set(PTHREAD_WIN_PKG_LIBRARIES_STA "pthreadVC2-s.lib")
    endif()
endif()

if(NOT WIN32)
  if(NOT DEFINED TANGO_CPPZMQ_USABLE_VERSION)
    try_compile(TANGO_CPPZMQ_USABLE_VERSION ${CMAKE_CURRENT_BINARY_DIR}/test_cppzmq_features
                SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/configure/test_cppzmq_features.cpp
                COMPILE_DEFINITIONS "-I ${TANGO_CPPZMQ_BASE}/include -I ${TANGO_ZMQ_BASE}/include"
                LINK_LIBRARIES "$<$<BOOL:${TANGO_ZMQ_BASE}>:-L ${TANGO_ZMQ_BASE}/lib >${ZMQ_PKG_LIBRARIES}"
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

  if(NOT DEFINED TANGO_ZMQ_USABLE_VERSION)
    try_compile(TANGO_ZMQ_USABLE_VERSION ${CMAKE_CURRENT_BINARY_DIR}/test_zmq_version
                SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/configure/test_zmq_version.cpp
                COMPILE_DEFINITIONS "-I ${TANGO_ZMQ_BASE}/include
                -DMINIMUM_VERSION_MAJOR=${ZMQ_MIN_VER_MAJOR}
                -DMINIMUM_VERSION_MINOR=${ZMQ_MIN_VER_MINOR}
                -DMINIMUM_VERSION_PATCH=${ZMQ_MIN_VER_PATCH}"
                LINK_LIBRARIES "$<$<BOOL:${TANGO_ZMQ_BASE}>:-L ${TANGO_ZMQ_BASE}/lib >${ZMQ_PKG_LIBRARIES}"
                OUTPUT_VARIABLE ZMQ_TRY_COMPILE_OUTPUT)
  endif()

  set(msg "Check if libzmq version is >= ${ZMQ_MIN_VER_MAJOR}.${ZMQ_MIN_VER_MINOR}.${ZMQ_MIN_VER_PATCH}: ${TANGO_ZMQ_USABLE_VERSION}")

  if(${TANGO_ZMQ_USABLE_VERSION})
    message(STATUS ${msg})
  else()
    message(FATAL_ERROR ${msg} "\n\nBuild output:\n" ${ZMQ_TRY_COMPILE_OUTPUT})
  endif()
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
  else()
    add_compile_options(-Werror)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wdeprecated -Wdeprecated-implementations -Wextra-semi)
    endif()
  endif()
endif()

include(GNUInstallDirs)

configure_file(tango.pc.cmake tango.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tango.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
