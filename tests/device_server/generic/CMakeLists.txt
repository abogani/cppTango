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
        FileDb.cpp
        classfactory.cpp
        DevTestClass.cpp
        DevTest.cpp)

add_executable(DevTest ${SOURCES})
target_link_libraries(DevTest PUBLIC tango ${CMAKE_DL_LIBS})
target_precompile_headers(DevTest REUSE_FROM conf_devtest)
