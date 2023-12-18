set(SOURCES main.cpp
        TypeCmds.cpp
        SigThrow.cpp
        IOMisc.cpp
        IOStr1.cpp
        IOStr2.cpp
        IOArray1.cpp
        IOArray2.cpp
        IOStrArray.cpp
        IOStruct.cpp
        IOSetAttr.cpp
        IOAddAttribute.cpp
        IOSeqVec.cpp
        classfactory.cpp
        DevTestClass.cpp
        DevTest.cpp)

tango_create_device_servers(DevTest ${SOURCES})
