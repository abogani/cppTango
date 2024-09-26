#[=======================================================================[.rst:
FindJPEG
---------

Find the JPEG library

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

    ``JPEG::JPEG``
    The JPEG library.  This will be either shared or static, with shared prefered.
    ``JPEG::JPEG-static``
    The JPEG static library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

    ``JPEG_FOUND``
    True if the required components have been found.
    ``JPEG_static_FOUND``
    True if the system has the C++ JPEG static library.
    ``JPEG_IS_STATIC``
    True if ``JPEG::JPEG`` and ``JPEG::JPEG-static`` are the same.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

    ``JPEG_INCLUDE_DIR``
    The directory containing ``libjpeg.hpp``.
    ``JPEG_shared_LIBRARY_RELEASE``
    The path to the release JPEG library.
    ``JPEG_shared_LIBRARY_DEBUG``
    The path to the debug JPEG library.
    ``JPEG_shared_LIBRARY``
    The path to the release JPEG library, or the debug library
    if the release library is not found
    ``JPEG_static_LIBRARY_RELEASE``
    The path to the release JPEG library.
    ``JPEG_static_LIBRARY_DEBUG``
    The path to the debug JPEG library.
    ``JPEG_static_LIBRARY``
    The path to the release JPEG library, or the debug library
    if the release library is not found
    ``JPEG_RUNTIME_RELEASE``
	The path to the release JPEG dll, windows only.
    ``JPEG_RUNTIME_DEBUG``
	The path to the debug JPEG dll, windows only.
#]=======================================================================]

find_path(JPEG_INCLUDE_DIR NAMES "jpeglib.h")

if(WIN32)
    set(_jpeg_lib_release_names "jpeg.lib")
    set(_jpeg_lib_debug_names "jpegd.lib")
    set(_jpeg_lib_static_release_names "jpeg-static.lib")
    set(_jpeg_lib_static_debug_names "jpeg-staticd.lib")
    set(_jpeg_runtime_release_names "jpeg62.dll")
    set(_jpeg_runtime_debug_names "jpeg62d.dll")
else()
    set(_jpeg_lib_release_names "jpeg")
    set(_jpeg_lib_debug_names "jpegd")
    set(_jpeg_lib_static_release_names "libjpeg.a")
    set(_jpeg_lib_static_debug_names "libjpegd.a")
endif(WIN32)

find_library(JPEG_LIBRARY_RELEASE
    NAMES ${_jpeg_lib_release_names}
    PATH_SUFFIXES Release
)

find_library(JPEG_LIBRARY_RELEASE
    NAMES ${_jpeg_lib_static_release_names}
    PATH_SUFFIXES Release
)

find_library(JPEG_LIBRARY_DEBUG
    NAMES ${_jpeg_lib_debug_names}
    PATH_SUFFIXES Debug
)

find_library(JPEG_LIBRARY_DEBUG
    NAMES ${_jpeg_lib_static_debug_names}
    PATH_SUFFIXES Debug
)

find_library(JPEG_static_LIBRARY_RELEASE
    NAMES ${_jpeg_lib_static_release_names}
    PATH_SUFFIXES Release
)

find_library(JPEG_static_LIBRARY_DEBUG
    NAMES ${_jpeg_lib_static_debug_names}
    PATH_SUFFIXES Debug
)

unset(_jpeg_lib_release_names)
unset(_jpeg_lib_debug_names)
unset(_jpeg_lib_static_release_names)
unset(_jpeg_lib_static_debug_names)

if(WIN32)
    find_file(JPEG_RUNTIME_DEBUG
        NAMES ${_jpeg_runtime_debug_names}
        PATH_SUFFIXES "bin/Debug" "bin"
    )
    find_file(JPEG_RUNTIME_RELEASE
        NAMES ${_jpeg_runtime_release_names}
        PATH_SUFFIXES "bin/Release" "bin"
    )
endif()

include(SelectLibraryConfigurations)
select_library_configurations(JPEG)
select_library_configurations(JPEG_static)

if (JPEG_static_LIBRARY)
    set(JPEG_static_FOUND TRUE)
endif()

if (JPEG_LIBRARY STREQUAL JPEG_static_LIBRARY)
    set(JPEG_IS_STATIC TRUE)
else()
    set(JPEG_IS_STATIC FALSE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JPEG
    REQUIRED_VARS
        JPEG_LIBRARY
        JPEG_INCLUDE_DIR
)

if (JPEG_FOUND)
    mark_as_advanced(JPEG_INCLUDE_DIR)
    mark_as_advanced(JPEG_LIBRARY)
    mark_as_advanced(JPEG_LIBRARY_RELEASE)
    mark_as_advanced(JPEG_LIBRARY_DEBUG)
endif()

if (JPEG_static_FOUND)
    mark_as_advanced(JPEG_static_LIBRARY)
    mark_as_advanced(JPEG_static_LIBRARY_RELEASE)
    mark_as_advanced(JPEG_static_LIBRARY_DEBUG)
endif()

if (JPEG_static_FOUND)
    if (NOT TARGET JPEG::JPEG-static)
        add_library(JPEG::JPEG-static STATIC IMPORTED)
    endif()
   if (JPEG_static_LIBRARY_RELEASE)
        set_property(TARGET JPEG::JPEG-static APPEND PROPERTY
            IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(JPEG::JPEG-static PROPERTIES
            IMPORTED_LOCATION_RELEASE "${JPEG_static_LIBRARY_RELEASE}")
    endif()
    if (JPEG_static_LIBRARY_DEBUG)
        set_property(TARGET JPEG::JPEG-static APPEND PROPERTY
            IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(JPEG::JPEG-static PROPERTIES
            IMPORTED_LOCATION_DEBUG "${JPEG_static_LIBRARY_DEBUG}")
    endif()
    set_target_properties(JPEG::JPEG-static PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${JPEG_INCLUDE_DIR}"
    )
endif()

if (JPEG_FOUND AND JPEG_IS_STATIC)
    if (NOT TARGET JPEG::JPEG)
        add_library(JPEG::JPEG ALIAS JPEG::JPEG-static)
    endif()
elseif(JPEG_FOUND)
    # If we are going to create the a SHARED IMPORTED target for the
    # release configuration, but we don't have the debug DLL then we should
    # not add a debug configuration.  We ensure that here by "unfinding"
    # any debug library we found.
    if (JPEG_RUNTIME_RELEASE AND JPEG_LIBRARY_RELEASE AND NOT JPEG_RUNTIME_DEBUG)
        set(JPEG_LIBRARY_DEBUG JPEG_LIBRARY_DEBUG-NOTFOUND)
    endif()

    # Similarly, if we are going to create a SHARED UNKNOWN target for the
    # release configuration, and we _have_ a debug DLL, we "unfind" the
    # debug DLL so that we don't use it.
    if (JPEG_LIBRARY_DEBUG AND JPEG_LIBRARY_RELEASE AND NOT JPEG_RUNTIME_RELEASE)
        set(JPEG_RUNTIME_DEBUG JPEG_RUNTIME_DEBUG-NOTFOUND)
    endif()

    if (JPEG_LIBRARY_RELEASE AND JPEG_RUNTIME_RELEASE)
        if (NOT TARGET JPEG::JPEG)
            add_library(JPEG::JPEG SHARED IMPORTED)
        endif()
        set_property(TARGET JPEG::JPEG APPEND PROPERTY
            IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(JPEG::JPEG PROPERTIES
            IMPORTED_LOCATION_RELEASE "${JPEG_RUNTIME_RELEASE}")
        set_target_properties(JPEG::JPEG PROPERTIES
            IMPORTED_IMPLIB_RELEASE "${JPEG_LIBRARY_RELEASE}")
    elseif(JPEG_LIBRARY_RELEASE)
        if (NOT TARGET JPEG::JPEG)
            add_library(JPEG::JPEG UNKNOWN IMPORTED)
        endif()
        set_property(TARGET JPEG::JPEG APPEND PROPERTY
            IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(JPEG::JPEG PROPERTIES
            IMPORTED_LOCATION_RELEASE "${JPEG_LIBRARY_RELEASE}")
    endif()

    if (JPEG_LIBRARY_DEBUG AND JPEG_RUNTIME_DEBUG)
        if (NOT TARGET JPEG::JPEG)
            add_library(JPEG::JPEG SHARED IMPORTED)
        endif()
        set_property(TARGET JPEG::JPEG APPEND PROPERTY
            IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(JPEG::JPEG PROPERTIES
            IMPORTED_LOCATION_DEBUG "${JPEG_RUNTIME_DEBUG}")
        set_target_properties(JPEG::JPEG PROPERTIES
            IMPORTED_IMPLIB_DEBUG "${JPEG_LIBRARY_DEBUG}")
    elseif(JPEG_LIBRARY_DEBUG)
        if (NOT TARGET JPEG::JPEG)
            add_library(JPEG::JPEG UNKNOWN IMPORTED)
        endif()
        set_property(TARGET JPEG::JPEG APPEND PROPERTY
            IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(JPEG::JPEG PROPERTIES
            IMPORTED_LOCATION_DEBUG "${JPEG_LIBRARY_DEBUG}")
    endif()
    set_target_properties(JPEG::JPEG PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${JPEG_INCLUDE_DIR}"
    )
endif()
