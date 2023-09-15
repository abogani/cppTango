# include guard
if(__get_git_revision_description)
  return()
endif()
set(__get_git_revision_description YES)

# Add a cmake configure dependency on git's HEAD and whatever it points to,
# starting each time from an empty set of dependencies. This will ensure we trigger
# a reconfiguration each time we move to a different commit
function(_add_cmake_configure_dependencies)
  set(GIT_DIR "${PROJECT_SOURCE_DIR}/.git")
  set(HEAD_FILE "${GIT_DIR}/HEAD")
  set(git_dependencies "")
  if(EXISTS "${HEAD_FILE}")
    list(APPEND git_dependencies "${HEAD_FILE}")
    file(READ "${HEAD_FILE}" HEAD_CONTENTS LIMIT 1024)
    string(STRIP "${HEAD_CONTENTS}" HEAD_CONTENTS)
    if(HEAD_CONTENTS MATCHES "ref")
      # named branch
      string(REPLACE "ref: " "" HEAD_REF "${HEAD_CONTENTS}")
      if(EXISTS "${GIT_DIR}/${HEAD_REF}")
        list(APPEND git_dependencies "${GIT_DIR}/${HEAD_REF}")
      endif()
    endif()
  endif()
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${git_dependencies})
endfunction()

_add_cmake_configure_dependencies()

# Custom cppTango logic to get git description and suffix
function(git_describe description_var suffix_var)
  set(GIT_DESCRIBE_STDOUT "unknown")
  set(SUFFIX_VERSION "")
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
    endif()
  endif()

  set(${description_var} ${GIT_DESCRIBE_STDOUT} PARENT_SCOPE)
  set(${suffix_var} ${SUFFIX_VERSION} PARENT_SCOPE)
endfunction()
