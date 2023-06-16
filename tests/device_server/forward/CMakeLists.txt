set(SOURCES ClassFactory.cpp
        FwdTestClass.cpp
        FwdTest.cpp
        FwdTestStateMachine.cpp
        main.cpp)

add_executable(FwdTest ${SOURCES})
target_link_libraries(FwdTest PUBLIC tango ${CMAKE_DL_LIBS})
if(TANGO_USE_PCH)
    target_precompile_headers(FwdTest REUSE_FROM conf_devtest)
endif()
