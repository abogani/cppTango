add_subdirectory(utils)
add_subdirectory(observability)

set(tango_version_h ${CMAKE_CURRENT_BINARY_DIR}/tango_version.h)
configure_file(tango_version.h.in ${tango_version_h})
set(COMMON_HEADERS
    ${tango_version_h}
    tango_const.h
    git_revision.h
)

install(FILES ${COMMON_HEADERS} DESTINATION
    "${CMAKE_INSTALL_INCLUDEDIR}/tango/common")
