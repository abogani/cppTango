set(SOURCES
        DummyClass.cpp
        DummyClassClass.cpp
        DummyClassStateMachine.cpp)

set(HEADERS
        DummyClass.h
        DummyClassClass.h)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(DummyClassObj OBJECT ${SOURCES} ${HEADERS})
target_link_libraries(DummyClassObj PRIVATE Tango::Tango ${CMAKE_DL_LIBS})

add_library(DummyClass SHARED $<TARGET_OBJECTS:DummyClassObj>)
target_link_libraries(DummyClass PRIVATE Tango::Tango ${CMAKE_DL_LIBS})

add_library(DummyClassPrefix SHARED $<TARGET_OBJECTS:DummyClassObj>)
target_link_libraries(DummyClassPrefix PRIVATE Tango::Tango ${CMAKE_DL_LIBS})

set_target_properties(DummyClassPrefix PROPERTIES OUTPUT_NAME "DummyClass" PREFIX "libtest" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/prefix)

add_library(DummyClassUnPrefix SHARED $<TARGET_OBJECTS:DummyClassObj>)
target_link_libraries(DummyClassUnPrefix PRIVATE Tango::Tango ${CMAKE_DL_LIBS})

set_target_properties(DummyClassUnPrefix PROPERTIES OUTPUT_NAME "DummyClass" PREFIX "" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/unprefix)
