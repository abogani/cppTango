set(SOURCES ClassFactory.cpp
        TestCppTango1022Class.cpp
        TestCppTango1022.cpp
        TestCppTango1022StateMachine.cpp
        TestCppTango1022DynAttrUtils.cpp
        main.cpp)

add_executable(TestCppTango1022 ${SOURCES})
target_link_libraries(TestCppTango1022 PUBLIC tango ${CMAKE_DL_LIBS})
if(TANGO_USE_PCH)
    tango_target_test_pch(TestCppTango1022)
    target_compile_definitions(TestCppTango1022 PRIVATE ${COMMON_TEST_CXX_DEFS})
endif()
