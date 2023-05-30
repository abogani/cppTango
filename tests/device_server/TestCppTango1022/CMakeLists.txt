set(SOURCES ClassFactory.cpp
        TestCppTango1022Class.cpp
        TestCppTango1022.cpp
        TestCppTango1022StateMachine.cpp
        TestCppTango1022DynAttrUtils.cpp
        main.cpp)

add_executable(TestCppTango1022 ${SOURCES})
target_link_libraries(TestCppTango1022 PUBLIC tango ${CMAKE_DL_LIBS})
target_precompile_headers(TestCppTango1022 REUSE_FROM conf_devtest)
target_compile_definitions(TestCppTango1022 PRIVATE ${COMMON_TEST_DEFS})
