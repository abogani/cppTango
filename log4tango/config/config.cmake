include(CheckIncludeFile)
include(FindThreads)
include(CheckCXXSourceCompiles)
include(CheckTypeSize)

macro (LOG4TANGO_CHECK_INCLUDE_FILE header var)
    CHECK_INCLUDE_FILE("${header}"        LOG4TANGO_${var} )
endmacro()

LOG4TANGO_CHECK_INCLUDE_FILE("io.h"           HAVE_IO_H)
LOG4TANGO_CHECK_INCLUDE_FILE("unistd.h"       HAVE_UNISTD_H)

if(NOT DEFINED Threads_FOUND)
  message(FATAL_ERROR "Could not find a suitable threading library")
endif()

configure_file(config/config.h.in ${PROJECT_BINARY_DIR}/log4tango/include/tango/common/log4tango/config.h)
