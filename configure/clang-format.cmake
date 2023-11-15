option(TANGO_CLANG_FORMAT "Add a target \"clang-format\" for formatting the C++ source code" OFF)

set(CLANG_FORMAT_REF_VERSION 17)

if(TANGO_CLANG_FORMAT)
  find_program(CLANG_FORMAT NAMES clang-format-17 clang-format)

  if(CLANG_FORMAT)
    execute_process(COMMAND "${CLANG_FORMAT}" --version
                    OUTPUT_VARIABLE CLANG_FORMAT_VERSION_STRING
                    OUTPUT_STRIP_TRAILING_WHITESPACE)


    string(REGEX
           # Examples:
           # clang-format version 16.0.6
           # Ubuntu clang-format version 16.0.6 (++20230710042046+7cbf1a259152-1~exp1~20230710162136.105)
           REPLACE ".*clang-format version ([0-9]+).*"
                   "\\1"
                   CLANG_FORMAT_MAJOR_VERSION
                   "${CLANG_FORMAT_VERSION_STRING}")

    if(CLANG_FORMAT_MAJOR_VERSION VERSION_LESS CLANG_FORMAT_REF_VERSION)
      message(FATAL_ERROR "The found clang-format version is ${CLANG_FORMAT_MAJOR_VERSION}, but we need at least ${CLANG_FORMAT_REF_VERSION}.")
    else()
      message(STATUS "Found clang-format version: ${CLANG_FORMAT_MAJOR_VERSION}")
    endif()

    file(GLOB_RECURSE SOURCE_LIST
         LIST_DIRECTORIES false
         RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
         log4tango/*.cpp
         log4tango/*.h
         log4tango/*.h.in
         src/*.cpp
         src/*.h
         src/*.h.in
         tests/*.cpp
         tests/*.h
         tests/*.h.in)

    add_custom_target(clang-format
                      COMMAND ${CLANG_FORMAT} "-i" "${SOURCE_LIST}"
                      COMMAND_EXPAND_LISTS
                      VERBATIM
                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  else()
    message(FATAL_ERROR "Could not find clang-format, target \"clang-format\" will be unavailable.\nSet TANGO_CLANG_FORMAT to OFF to avoid creating the target.")
  endif()
endif()
