#[=======================================================================[.rst:
Findtangoidl
---------

Find the tango IDL file

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

    ``tangoidl::tangoidl``
    The tango IDL file

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

    ``tangoidl_FOUND``
    True if the system has the tango IDL file.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

    ``tangoidl_INCLUDE_DIR``
    The directory containing ``tango.idl``.

#]=======================================================================]

find_package(tangoidl CONFIG QUIET)

if (NOT TARGET tangoidl::tangoidl)
    if (NOT DEFINED PKG_CONFIG_FOUND)
        find_package(PkgConfig QUIET)
    endif()

    # Collect hints from pkg-config
    if (PKG_CONFIG_FOUND)
        pkg_search_module(_tangoidl_PKG tangoidl QUIET)
    endif()

    find_path(tangoidl_INCLUDE_DIR
        NAMES tango.idl
        PATHS "${_tangoidl_PKG_INCLUDE_DIRS}"
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(tangoidl
        REQUIRED_VARS
            tangoidl_INCLUDE_DIR
    )

    if (tangoidl_FOUND)
        mark_as_advanced(tangoidl_INCLUDE_DIR)
    endif()

    if (tangoidl_FOUND AND NOT TARGET tangoidl::tangoidl)
        add_library(tangoidl::tangoidl INTERFACE IMPORTED)
        set_target_properties(tangoidl::tangoidl PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${tangoidl_INCLUDE_DIR}"
        )
    endif()
endif()
