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

set_property(TARGET tango PROPERTY LINK_FLAGS "/DEBUG")

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

# This will contain cmake code to recreate all the IMPORTED targets used
# by Tango
set(TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS "")

# Install the lib/dll files for a target and add code to
# TANGO_INSTALL_DEPENDENCIES_IMPORTED_TARGETS to reconstitute the IMPORTED target
# once it has been installed.
function(tango_install_imported tgt)
    get_target_property(type ${tgt} TYPE)
    get_target_property(imported_cfgs ${tgt} IMPORTED_CONFIGURATIONS)
    if (imported_cfgs)
        foreach(cfg ${imported_cfgs})
            string(TOUPPER ${cfg} cfg)
            get_target_property(loc_${cfg} ${tgt} IMPORTED_LOCATION_${cfg})
            get_target_property(implib_${cfg} ${tgt} IMPORTED_IMPLIB_${cfg})
            get_filename_component(loc_name_${cfg} ${loc_${cfg}} NAME)
            get_filename_component(implib_name_${cfg} ${implib_${cfg}} NAME)
        endforeach()

        list(GET imported_cfgs 0 fallback_cfg)
    else()
        set(fallback_cfg NONE)
    endif()

    if(${type} STREQUAL SHARED_LIBRARY)
        set(LOCDIR ${CMAKE_INSTALL_BINDIR})
    else()
        set(LOCDIR ${CMAKE_INSTALL_LIBDIR})
    endif()

    foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${cfg} cfg)
        if (loc_${cfg})
            install(FILES ${loc_${cfg}}
                CONFIGURATIONS ${cfg}
                DESTINATION ${LOCDIR}
            )
        elseif(loc_${fallback_cfg})
            install(FILES ${loc_${fallback_cfg}}
                CONFIGURATIONS ${cfg}
                DESTINATION ${LOCDIR}
            )
        endif()

        if (implib_${cfg})
            install(FILES ${implib_${cfg}}
                CONFIGURATIONS ${cfg}
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )
        elseif(implib_${fallback_cfg})
            install(FILES ${implib_${fallback_cfg}}
                CONFIGURATIONS ${cfg}
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )
        endif()
    endforeach()

    if(${type} STREQUAL SHARED_LIBRARY)
        STRING(APPEND TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS "\
add_library(${tgt} SHARED IMPORTED)\n")
    elseif(${type} STREQUAL STATIC_LIBRARY)
        STRING(APPEND TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS "\
add_library(${tgt} STATIC IMPORTED)\n")
    else()
        set(has_loc FALSE)

        foreach(cfg ${imported_cfgs})
            if (loc_${cfg})
                set(has_loc TRUE)
            endif()
        endforeach()

        if (has_loc)
            STRING(APPEND TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS "\
add_library(${tgt} UNKNOWN IMPORTED)\n")
        else()
            STRING(APPEND TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS "\
add_library(${tgt} INTERFACE IMPORTED)\n")
        endif()
    endif()

    # As we are installing all the include files to the same directory
    # we do not need to copy an INTERFACE_INCLUDE_DIRECTORIES property
    set(props_to_check
        INTERFACE_LINK_LIBRARIES
        INTERFACE_LINK_OPTIONS
        STATIC_LIBRARY_OPTIONS
        INTERFACE_COMPILE_FEATURES
        INTERFACE_COMPILE_DEFINITIONS
        INTERFACE_COMPILE_OPTIONS
        )

    set(props_copy_lines "")
    foreach (prop ${props_to_check})
        get_target_property(value ${tgt} ${prop})
        if (value)
            string(APPEND props_copy_lines "    ${prop} \"${value}\"\n")
        endif()
    endforeach()

    if (props_copy_lines)
        STRING(APPEND TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS "\
set_target_properties(${tgt} PROPERTIES\n\
${props_copy_lines}    )\n")
    endif()

    if (${type} STREQUAL SHARED_LIBRARY)
        foreach(cfg ${imported_cfgs})
            string(TOUPPER ${cfg} cfg)
            if (loc_${cfg} AND implib_${cfg})
                STRING(APPEND TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS "\
    if(EXISTS \${TANGO_IMPORT_BINDIR}/${loc_name_${cfg}})\n\
        set_property(TARGET ${tgt} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${cfg})\n\
        set_target_properties(${tgt} PROPERTIES\n\
            IMPORTED_LOCATION_${cfg} \${TANGO_IMPORT_BINDIR}/${loc_name_${cfg}}\n\
            IMPORTED_IMPLIB_${cfg} \${TANGO_IMPORT_LIBDIR}/${implib_name_${cfg}}\n\
            )\n\
    endif()\n")
            endif()
        endforeach()
    else()
        foreach(cfg ${imported_cfgs})
            string(TOUPPER ${cfg} cfg)
            if (loc_${cfg})
                STRING(APPEND TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS "\
if(EXISTS \${TANGO_IMPORT_LIBDIR}/${loc_name_${cfg}})\n\
    set_property(TARGET ${tgt} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${cfg})\n\
    set_target_properties(${tgt} PROPERTIES\n\
        IMPORTED_LOCATION_${cfg} \${TANGO_IMPORT_LIBDIR}/${loc_name_${cfg}}\n\
        )\n\
endif()\n")
            endif()
        endforeach()
    endif()

    STRING(APPEND TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS "\n")
    set(TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS ${TANGO_INSTALLED_DEPENDENCIES_IMPORTED_TARGETS} PARENT_SCOPE)
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
            # This is a bit of hack because ZLIB::ZLIB is awkwardly defined as a UNKNOWN library
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
