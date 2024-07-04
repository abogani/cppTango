if(CMAKE_CL_64)
    add_definitions(-D_64BITS)
endif()

# multi process compilation
add_compile_options(/MP)

set(TANGO_LIBRARY_NAME tango)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TANGO_LIBRARY_NAME ${TANGO_LIBRARY_NAME}d)
endif()

if(BUILD_SHARED_LIBS)
    set_target_properties(tango PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    set(TANGO_LIBRARY_NAME ${TANGO_LIBRARY_NAME}-static)
endif()

message("Tango library is '${TANGO_LIBRARY_NAME}'")

#include and link directories

set(WIN32_LIBS "ws2_32.lib;mswsock.lib;advapi32.lib;comctl32.lib;odbc32.lib;")

set_target_properties(tango PROPERTIES
    COMPILE_DEFINITIONS "${windows_defs}"
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${SO_VERSION}
    DEBUG_POSTFIX "d")

set_cflags_and_include(tango)

if(NOT BUILD_SHARED_LIBS)
    set_target_properties(tango PROPERTIES
        PREFIX "lib"
        OUTPUT_NAME ${TANGO_LIBRARY_NAME})
endif()

# Always generate separate PDB files for shared builds, even for release build types
#
# https://docs.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format
# https://docs.microsoft.com/en-us/cpp/build/reference/debug-generate-debug-info
target_compile_options(tango PRIVATE "/Zi")

target_link_libraries(tango
    PRIVATE
        ${WIN32_LIBS})

set_property(TARGET tango PROPERTY LINK_FLAGS "/force:multiple /DEBUG")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    set(CMAKE_INSTALL_CONFIG_NAME Debug)
endif()

#install code

install(TARGETS tango
        EXPORT TangoTargets
        ARCHIVE DESTINATION lib COMPONENT static
        RUNTIME DESTINATION bin COMPONENT dynamic)

install(DIRECTORY "$<TARGET_FILE_DIR:tango>/"
        DESTINATION lib COMPONENT static
        DESTINATION bin COMPONENT dynamic
        FILES_MATCHING PATTERN "*.pdb")

# Install the header files for the target
function(tango_install_dependency_headers tgt)
    if (NOT TARGET ${tgt})
        return()
    endif()

    get_target_property(inc_dirs ${tgt} INTERFACE_INCLUDE_DIRECTORIES)
    foreach(dir ${inc_dirs})
        file(GLOB files "${dir}/*")
        foreach (file ${files})
            if (IS_DIRECTORY ${file})
                install(DIRECTORY ${file} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING
                    REGEX ".*" REGEX "\\.in" EXCLUDE)
            elseif(NOT ${file} MATCHES "\\.in$")
                install(FILES ${file} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
            endif()
        endforeach()
    endforeach()
endfunction()

# Install the lib/dll files for a target and add code to
# TANGO_INSTALL_DEPENDENCIES_IMPORTED_TARGETS to reconstitute the IMPORTED target
# once it has been installed.
function(tango_install_imported tgt)
    get_target_property(type ${tgt} TYPE)
    get_target_property(loc_rel ${tgt} IMPORTED_LOCATION_RELEASE)
    get_target_property(loc_dbg ${tgt} IMPORTED_LOCATION_DEBUG)
    get_target_property(implib_rel ${tgt} IMPORTED_IMPLIB_RELEASE)
    get_target_property(implib_dbg ${tgt} IMPORTED_IMPLIB_DEBUG)

    get_filename_component(loc_rel_name ${loc_rel} NAME)
    get_filename_component(loc_dbg_name ${loc_dbg} NAME)
    get_filename_component(implib_rel_name ${implib_rel} NAME)
    get_filename_component(implib_dbg_name ${implib_dbg} NAME)

    if(${type} STREQUAL SHARED_LIBRARY)
        set(LOCDIR ${CMAKE_INSTALL_BINDIR})
    else()
        set(LOCDIR ${CMAKE_INSTALL_LIBDIR})
    endif()

    if (loc_rel)
        install(FILES  ${loc_rel}
            CONFIGURATIONS Release
            DESTINATION ${LOCDIR}
        )
    endif()
    if(implib_rel)
        install(FILES  ${implib_rel}
            CONFIGURATIONS Release
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()
    if (loc_dbg)
        install(FILES  ${loc_dbg}
            CONFIGURATIONS Debug
            DESTINATION ${LOCDIR}
        )
    endif()
    if (implib_dbg)
        install(FILES  ${implib_dbg}
            CONFIGURATIONS Debug
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()

endfunction()

# Recursively look through INTERFACE_LINK_LIBRARIES targets of `tgt` and append
# them to the list `out` in the PARENT_SCOPE.
function(tango_gather_dependencies out tgt)
    if (NOT TARGET ${tgt} OR ${tgt} IN_LIST ${out})
        return()
    endif()
    list(APPEND ${out} ${tgt})
    get_target_property(iface_link_libs ${tgt} INTERFACE_LINK_LIBRARIES)
    if (iface_link_libs)
        foreach (dep ${iface_link_libs})
            string(REGEX REPLACE "\\\$<LINK_ONLY:(.*)>" "\\1" stripped ${dep})
            tango_gather_dependencies(${out} ${stripped})
        endforeach()
    endif()
    set(${out} ${${out}} PARENT_SCOPE)
endfunction()

if (TANGO_INSTALL_DEPENDENCIES)
    # We want to install all the lib/dll files for _all_ the targets Tango::Tango depends on
    get_target_property(TANGO_LINK_LIBS tango LINK_LIBRARIES)
    foreach (tgt ${TANGO_LINK_LIBS})
        tango_gather_dependencies(TANGO_LINK_DEPENDENCIES ${tgt})
    endforeach()

    foreach (tgt ${TANGO_LINK_DEPENDENCIES})
        tango_install_imported(${tgt})
    endforeach()

    # We only want to install the include files for the INTERFACE_LIBRARIES
    get_target_property(TANGO_IFACE_LIBS tango INTERFACE_LINK_LIBRARIES)
    foreach (tgt ${TANGO_IFACE_LIBS})
        tango_gather_dependencies(TANGO_HEADER_DEPENDENCIES ${tgt})
    endforeach()

    foreach(tgt ${TANGO_HEADER_DEPENDENCIES})
        tango_install_dependency_headers(${tgt})
    endforeach()

    if(TANGO_USE_TELEMETRY)
        if(BUILD_SHARED_LIBS)
          message(FATAL_ERROR "Missing installation code")
        else()
            # This is a bit of hack because ZLIB:ZLIB is awkwardly defined as a UNKNOWN library
            # and doesn't know about the DLL, so our logic above misses it.
            # TODO: Somehow avoid this
          if (NOT ZLIB_ROOT)
            get_filename_component(ZLIB_LIBRARY_DIR ${ZLIB_LIBRARY} DIRECTORY)
            get_filename_component(ZLIB_ROOT ${ZLIB_LIBRARY_DIR} DIRECTORY)
          endif()

          install(FILES ${ZLIB_ROOT}/bin/zlib1.dll DESTINATION ${CMAKE_INSTALL_BINDIR})
      endif()
    endif()
endif()
