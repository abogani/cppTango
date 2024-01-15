set(SOURCES ClassFactory.cpp
        TestCppTango1022Class.cpp
        TestCppTango1022.cpp
        TestCppTango1022StateMachine.cpp
        TestCppTango1022DynAttrUtils.cpp
        main.cpp)

tango_create_device_servers(TestCppTango1022 ${SOURCES})
