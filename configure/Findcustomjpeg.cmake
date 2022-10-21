if(TANGO_JPEG_BASE)
    find_path(JPEG_INCLUDE jpeglib.h PATHS ${TANGO_JPEG_BASE}/include)

    if(WIN32)
        find_file(JPEG_IMPLIB_DEBUG NAMES jpegd.lib PATHS ${TANGO_JPEG_BASE}/lib)

        find_file(JPEG_LIB_DEBUG NAMES jpeg62d.dll PATHS ${TANGO_JPEG_BASE}/lib ${TANGO_JPEG_BASE}/bin)

        find_file(JPEG_LIB_STATIC_DEBUG NAMES jpegd-static.lib PATHS ${TANGO_JPEG_BASE}/lib)

        find_file(JPEG_IMPLIB NAMES jpeg.lib PATHS ${TANGO_JPEG_BASE}/lib)

        find_file(JPEG_LIB NAMES jpeg62.dll PATHS ${TANGO_JPEG_BASE}/lib ${TANGO_JPEG_BASE}/bin)

        find_file(JPEG_LIB_STATIC NAMES jpeg-static.lib PATHS ${TANGO_JPEG_BASE}/lib)

    else(WIN32)
        find_file(JPEG_LIB NAMES libjpeg.so PATHS ${TANGO_JPEG_BASE}/lib ${TANGO_JPEG_BASE}/lib64)
        
        find_file(JPEG_LIB_DEBUG NAMES libjpeg.so PATHS ${TANGO_JPEG_BASE}/lib ${TANGO_JPEG_BASE}/lib64)

        find_file(JPEG_LIB_STATIC NAMES libjpeg.a PATHS ${TANGO_JPEG_BASE}/lib ${TANGO_JPEG_BASE}/lib64)
        
        find_file(JPEG_LIB_STATIC_DEBUG NAMES libjpeg.a PATHS ${TANGO_JPEG_BASE}/lib ${TANGO_JPEG_BASE}/lib64)
    endif(WIN32)

    if(NOT ${JPEG_INCLUDE} STREQUAL "JPEG_INCLUDE-NOTFOUND")
        # Use TANGO_JPEG_BASE for the target
        add_library(jpeg SHARED IMPORTED)

        add_library(jpeg-static STATIC IMPORTED)

        set_target_properties(jpeg
            PROPERTIES
                IMPORTED_LOCATION_RELEASE ${JPEG_LIB}
                IMPORTED_LOCATION_DEBUG ${JPEG_LIB_DEBUG}
                INTERFACE_INCLUDE_DIRECTORIES ${JPEG_INCLUDE}
                IMPORTED_CONFIGURATIONS "RELEASE;DEBUG")

        set_target_properties(jpeg-static
            PROPERTIES
                IMPORTED_LOCATION_RELEASE ${JPEG_LIB_STATIC}
                IMPORTED_LOCATION_DEBUG ${JPEG_LIB_STATIC_DEBUG}
                INTERFACE_INCLUDE_DIRECTORIES ${JPEG_INCLUDE}
                IMPORTED_CONFIGURATIONS "RELEASE;DEBUG")

        if(WIN32)
            set_target_properties(jpeg
                PROPERTIES
                    IMPORTED_IMPLIB_RELEASE ${JPEG_IMPLIB}
                    IMPORTED_IMPLIB_DEBUG ${JPEG_IMPLIB_DEBUG})

        endif(WIN32)

        message(STATUS "Configured jpeg")

        set(jpeg_INCLUDEDIR ${JPEG_INCLUDE})
        set(customjpeg_FOUND TRUE)
    else(NOT ${JPEG_INCLUDE} STREQUAL "JPEG_INCLUDE-NOTFOUND")
        if(customjpeg_FIND_REQUIRED)
            message(FATAL_ERROR "Could not find jpeg in ${TANGO_JPEG_BASE}.\n"
                "If you installed it in the standard location try without using TANGO_JPEG_BASE")
        endif(customjpeg_FIND_REQUIRED)
    endif(NOT ${JPEG_INCLUDE} STREQUAL "JPEG_INCLUDE-NOTFOUND")

else(TANGO_JPEG_BASE)

    find_package(JPEG QUIET)

    if(TARGET JPEG::JPEG)
        add_library(jpeg INTERFACE IMPORTED)
        target_link_libraries(jpeg INTERFACE JPEG::JPEG)
        get_target_property(JPEG_INCLUDE JPEG::JPEG INTERFACE_INCLUDE_DIRECTORIES)
        set_target_properties(jpeg
            PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${JPEG_INCLUDE})
        set(jpeg_INCLUDEDIR ${JPEG_INCLUDE})
    endif(TARGET JPEG::JPEG)

    if(NOT JPEG_FOUND)
        # Ensure pkg-config is installed
        if(NOT PkgConfig_FOUND)
            find_package(PkgConfig)
        endif(NOT PkgConfig_FOUND)
	    
        message(STATUS "Search for libjpeg with package config...")
	    
        pkg_check_modules(JPEG REQUIRED libjpeg IMPORTED_TARGET GLOBAL)
		
        if(JPEG_FOUND)
            # Create libraries
            add_library(jpeg ALIAS PkgConfig::JPEG)
            
            set(jpeg_INCLUDEDIR ${JPEG_INCLUDEDIR})
            
            message(STATUS "Configured jpeg via pkgconfig")
        else(JPEG_FOUND)
	    
            if(customjpeg_FIND_REQUIRED)
                message(FATAL_ERROR "Could not find libjpeg.\n"
                    "Check it is installed or try providing its path through TANGO_JPEG_BASE")
            endif(customjpeg_FIND_REQUIRED)
	    
        endif(JPEG_FOUND)
    endif(NOT JPEG_FOUND)
	
    set(customjpeg_FOUND JPEG_FOUND)

endif(TANGO_JPEG_BASE)
