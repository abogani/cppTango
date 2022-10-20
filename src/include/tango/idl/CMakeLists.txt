get_target_property(IDL_PKG_INCLUDE_DIRS tangoidl::tangoidl INTERFACE_INCLUDE_DIRECTORIES)

message("Using OMNIIDL_PATH=${OMNIIDL_PATH}")
message("Using IDL=${IDL_PKG_INCLUDE_DIRS}")

add_custom_command(OUTPUT tango.h tangoSK.cpp tangoDynSK.cpp
                   COMMAND ${OMNIIDL} -I${IDL_PKG_INCLUDE_DIRS} -bcxx -Wbh=.h -Wbs=SK.cpp -Wbd=DynSK.cpp -Wba ${IDL_PKG_INCLUDE_DIRS}/tango.idl
                   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/apply-patches.cmake
                   COMMENT "Generate tango.h, tangoSK.cpp and tangoDynSK.cpp from idl"
                   DEPENDS ${IDL_PKG_INCLUDE_DIRS}/tango.idl
                           ${CMAKE_CURRENT_SOURCE_DIR}/apply-patches.cmake
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES ${CMAKE_CURRENT_BINARY_DIR}/tangoSK.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/tangoDynSK.cpp)

if(WIN32)
    foreach(file ${SOURCES})
        set_source_files_properties(${file} PROPERTIES COMPILE_DEFINITIONS _OMNIORB_STUB_DLL)
        set_source_files_properties(${file} PROPERTIES COMPILE_DEFINITIONS __NT__)
    endforeach(file)
endif()

add_custom_target(idl_source DEPENDS ${BUILD_TANGO_LOCAL_INCLUDE_DIR}/tango/tango.h tangoSK.cpp tangoDynSK.cpp)

add_library(idl_objects OBJECT ${SOURCES})
target_include_directories(idl_objects SYSTEM PUBLIC ${OMNIORB_PKG_INCLUDE_DIRS})
add_dependencies(idl_objects idl_source)

if(WIN32)
    target_compile_definitions(idl_objects PRIVATE "${windows_defs};__x86__;__NT__;__OSVERSION__=4;__WIN32__;_WIN32_WINNT=0x0400;")
else()
    target_compile_options(idl_objects PRIVATE -fPIC)
    target_compile_definitions(idl_objects PRIVATE OMNI_UNLOADABLE_STUBS)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      target_compile_options(idl_objects PRIVATE -Wno-maybe-uninitialized -Wno-unused-parameter)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      # remove -Wno-deprecated once https://gitlab.com/tango-controls/cppTango/-/issues/786 is resolved
      target_compile_options(idl_objects PRIVATE -Wno-sometimes-uninitialized -Wno-unused-variable -Wno-unused-parameter -Wno-deprecated)
    endif()

endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tango.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tango/idl)
