set(LOG4TANGO_TEST_SOURCES test_log4tango.cpp)
set(BENCH_TEST_SOURCES clock.cpp clock.h test_bench.cpp)

add_executable(test_log4tango ${LOG4TANGO_TEST_SOURCES} $<TARGET_OBJECTS:log4tango_objects>)
add_executable(test_bench ${BENCH_TEST_SOURCES} $<TARGET_OBJECTS:log4tango_objects>)

target_include_directories(test_log4tango PRIVATE
    ${LOG4TANGO_SOURCE_DIR}/include
    ${LOG4TANGO_BINARY_DIR}/include
)

target_include_directories(test_bench PRIVATE
    ${LOG4TANGO_SOURCE_DIR}/include
    ${LOG4TANGO_BINARY_DIR}/include
)

if(WIN32)
    target_compile_definitions(test_log4tango PRIVATE "${static_defs}")
    target_compile_definitions(test_bench PRIVATE "${static_defs}")
endif()

add_test("log4tango_test"       test_log4tango)
add_test("log4tango_benchmark"  test_bench)
