function(_tango_create_single_device_server targetName)
    add_executable(${targetName} ${ARGN})
    target_link_libraries(${targetName} PUBLIC tango ${CMAKE_DL_LIBS})
    target_precompile_headers(${targetName} REUSE_FROM conf_devtest)
    target_compile_definitions(${targetName} PRIVATE ${COMMON_TEST_DEFS})
endfunction()

function(tango_create_device_servers targetName)
    _tango_create_single_device_server(${targetName} ${ARGN})

    set(IDL_VER 5)
    _tango_create_single_device_server(${targetName}_IDL${IDL_VER} ${ARGN})
    target_compile_definitions(${targetName}_IDL${IDL_VER}
        PRIVATE "-DTANGO_TEST_BASE_CLASS=Tango::Device_${IDL_VER}Impl")
endfunction()

add_subdirectory(forward)
add_subdirectory(generic)
add_subdirectory(TestCppTango1022)
