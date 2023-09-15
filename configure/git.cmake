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
