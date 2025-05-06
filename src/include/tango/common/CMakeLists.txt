add_subdirectory(utils)
add_subdirectory(telemetry)

set(tango_version_h ${CMAKE_CURRENT_BINARY_DIR}/tango_version.h)
configure_file(tango_version.h.in ${tango_version_h})
set(COMMON_HEADERS
    ${tango_version_h}
    tango_base_class.h
    tango_const.h
    tango_type_traits.h
    git_revision.h
    pointer_with_lock.h
    omnithread_wrapper.h
)

install(FILES ${COMMON_HEADERS} DESTINATION
    "${CMAKE_INSTALL_INCLUDEDIR}/tango/common")
