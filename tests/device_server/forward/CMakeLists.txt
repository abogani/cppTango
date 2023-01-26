set(SOURCES ClassFactory.cpp
        FwdTestClass.cpp
        FwdTest.cpp
        FwdTestStateMachine.cpp
        main.cpp)

add_executable(FwdTest ${SOURCES})
target_link_libraries(FwdTest PUBLIC tango ${CMAKE_DL_LIBS})
if(TANGO_USE_PCH)
    tango_target_test_pch(FwdTest)
    target_compile_definitions(FwdTest PRIVATE ${COMMON_TEST_DEFS})
endif()
