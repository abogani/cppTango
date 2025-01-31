set(COMMON_TEST_DEFS _REENTRANT)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/check_from_chars_double.cpp [===[
#include <charconv>

int main()
{
    double value;
    std::from_chars(nullptr, nullptr, value);
    return 0;
}

]===])

try_compile(
    TANGO_HAS_FROM_CHARS_DOUBLE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/check_from_chars_double.cpp)

if(${TANGO_HAS_FROM_CHARS_DOUBLE})
    list(APPEND COMMON_TEST_DEFS TANGO_HAS_FROM_CHARS_DOUBLE)
endif()

include(old_cxx_tests.cmake)

include(catch2/tango_catch2_tests.cmake)

tango_catch2_tests_create(
    catch2_alarm_event.cpp
    catch2_alarm.cpp
    catch2_attr_async_cb.cpp
    catch2_attr_manip.cpp
    catch2_attr_proxy.cpp
    catch2_attr_polling.cpp
    catch2_attr_read_write_simple.cpp
    catch2_cmd_polling.cpp
    catch2_connection.cpp
    catch2_test_dtypes.cpp
    catch2_dev_state.cpp
    catch2_internal_utils.cpp
    catch2_internal_stl_helpers.cpp
    catch2_misc.cpp
    catch2_nodb_connection.cpp
    catch2_change_event_on_nan.cpp
    catch2_synchronised_queue.cpp
    $<$<BOOL:${TANGO_USE_TELEMETRY}>:${CMAKE_CURRENT_SOURCE_DIR}/catch2_telemetry.cpp>
    catch2_tango_monitor.cpp
    catch2_unit_test_device_data.cpp
    catch2_w_attribute_set_write_value.cpp
    # These currrently fail on Windows and need investigating
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:catch2_filedatabase.cpp>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:catch2_jpeg_encoding.cpp>
    catch2_loggerstream_attribute.cpp
    )
