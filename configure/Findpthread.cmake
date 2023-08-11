#[=======================================================================[.rst:
Findpthread
---------

Find the pthread library

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

    ``pthread::pthread``
    The pthread library.
    ``pthread::pthread-static``
    The pthread static library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

    ``pthread_FOUND``
    True if the required components have been found.
    ``pthread_static_FOUND``
    True if the system has the C++ pthread static library.
    ``pthread_IS_STATIC``
    True if ``pthread::pthread`` and ``pthread::pthread-static`` are the same.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

    ``pthread_INCLUDE_DIR``
    The directory containing ``pthread.h``.
    ``pthread_shared_LIBRARY_RELEASE``
    The path to the release pthread library.
    ``pthread_shared_LIBRARY_DEBUG``
    The path to the debug pthread library.
    ``pthread_shared_LIBRARY``
    The path to the release pthread library, or the debug library
    if the release library is not found
    ``pthread_static_LIBRARY_RELEASE``
    The path to the release pthread library.
    ``pthread_static_LIBRARY_DEBUG``
    The path to the debug pthread library.
    ``pthread_static_LIBRARY``
    The path to the release pthread library, or the debug library
    if the release library is not found

#]=======================================================================]

find_path(pthread_INCLUDE_DIR
    NAMES "pthread.h"
    PATHS ""
)

find_library(pthread_LIBRARY_RELEASE
    NAMES "pthreadVC2"
)

find_library(pthread_LIBRARY_DEBUG
    NAMES "pthreadVC2d"
)

find_library(pthread_static_LIBRARY_RELEASE
    NAMES "pthreadVC2-s"
    NAMES_PER_DIR
)

find_library(pthread_static_LIBRARY_DEBUG
    NAMES "pthreadVC2-sd"
    NAMES_PER_DIR
)

include(SelectLibraryConfigurations)
select_library_configurations(pthread)
select_library_configurations(pthread_static)

if (pthread_static_LIBRARY)
    set(pthread_static_FOUND TRUE)
endif()

if (pthread_LIBRARY STREQUAL pthread_static_LIBRARY)
    set(pthread_IS_STATIC TRUE)
else()
    set(pthread_IS_STATIC FALSE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pthread
    REQUIRED_VARS
        pthread_LIBRARY
        pthread_INCLUDE_DIR
)

if (pthread_FOUND)
    mark_as_advanced(pthread_INCLUDE_DIR)
    mark_as_advanced(pthread_LIBRARY)
    mark_as_advanced(pthread_LIBRARY_RELEASE)
    mark_as_advanced(pthread_LIBRARY_DEBUG)
endif()

if (pthread_static_FOUND)
    mark_as_advanced(pthread_static_LIBRARY)
    mark_as_advanced(pthread_static_LIBRARY_RELEASE)
    mark_as_advanced(pthread_static_LIBRARY_DEBUG)
endif()

if (pthread_FOUND)
    if (NOT TARGET pthread::pthread)
        add_library(pthread::pthread UNKNOWN IMPORTED)
    endif()
    if (pthread_LIBRARY_RELEASE)
        set_property(TARGET pthread::pthread APPEND PROPERTY
            IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(pthread::pthread PROPERTIES
            IMPORTED_LOCATION_RELEASE "${pthread_LIBRARY_RELEASE}")
    endif()
    if (pthread_LIBRARY_DEBUG)
        set_property(TARGET pthread::pthread APPEND PROPERTY
            IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(pthread::pthread PROPERTIES
            IMPORTED_LOCATION_DEBUG "${pthread_LIBRARY_DEBUG}")
    endif()
    set_target_properties(pthread::pthread PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${pthread_INCLUDE_DIR}"
        INTERFACE_COMPILE_DEFINITIONS "-DHAVE_PTHREAD_NP_H"
    )
endif()

if (pthread_static_FOUND)
    if (NOT TARGET pthread::pthread-static)
        add_library(pthread::pthread-static UNKNOWN IMPORTED)
    endif()
   if (pthread_static_LIBRARY_RELEASE)
        set_property(TARGET pthread::pthread-static APPEND PROPERTY
            IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(pthread::pthread-static PROPERTIES
            IMPORTED_LOCATION_RELEASE "${pthread_static_LIBRARY_RELEASE}")
    endif()
    if (pthread_static_LIBRARY_DEBUG)
        set_property(TARGET pthread::pthread-static APPEND PROPERTY
            IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(pthread::pthread-static PROPERTIES
            IMPORTED_LOCATION_DEBUG "${pthread_static_LIBRARY_DEBUG}")
    endif()
    set_target_properties(pthread::pthread-static PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${pthread_INCLUDE_DIR}"
        INTERFACE_COMPILE_DEFINITIONS "-DHAVE_PTHREAD_NP_H"
    )
endif()
