set(SOURCES ClassFactory.cpp
        TestCppTango1022Class.cpp
        TestCppTango1022.cpp
        TestCppTango1022StateMachine.cpp
        TestCppTango1022DynAttrUtils.cpp
        main.cpp)

add_executable(TestCppTango1022 ${SOURCES})
target_link_libraries(TestCppTango1022 PUBLIC tango ${CMAKE_DL_LIBS})
if(TANGO_USE_PCH)
    target_precompile_headers(TestCppTango1022 REUSE_FROM conf_devtest)
endif()
