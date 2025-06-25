set(COMMON_TEST_DEFS _REENTRANT)

if (WIN32)
    list(APPEND COMMON_TEST_DEFS NOMINMAX)
endif()

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

find_package(nlohmann_json QUIET)

if (NOT nlohmann_json_FOUND)
    message(STATUS "JSON library not found, skipping QueryEventSystem() test")
endif()

tango_catch2_tests_create(
    catch2_alarm_event.cpp
    catch2_alarm.cpp
    catch2_attr_async_cb.cpp
    catch2_attr_manip.cpp
    catch2_attr_proxy.cpp
    catch2_attr_conf_event.cpp
    catch2_attr_polling.cpp
    catch2_attr_read_write_simple.cpp
    catch2_cmd_polling.cpp
    catch2_cmd_query.cpp
    catch2_connection.cpp
    catch2_data_ready_event.cpp
    catch2_dev_intr_event.cpp
    catch2_event_on_connection_failure.cpp
    catch2_test_dtypes.cpp
    catch2_state_status_events.cpp
    catch2_dev_state.cpp
    catch2_error_in_event_callback.cpp
    catch2_internal_utils.cpp
    catch2_internal_stl_helpers.cpp
    catch2_misc.cpp
    catch2_multi_thread_sighandler.cpp
    catch2_nodb_connection.cpp
    catch2_change_event_on_nan.cpp
    catch2_server.cpp
    catch2_synchronised_queue.cpp
    catch2_configure_zmq_ports.cpp
    $<$<BOOL:${nlohmann_json_FOUND}>:${CMAKE_CURRENT_SOURCE_DIR}/catch2_query_event_system.cpp>
    $<$<BOOL:${TANGO_USE_TELEMETRY}>:${CMAKE_CURRENT_SOURCE_DIR}/catch2_telemetry.cpp>
    catch2_tango_monitor.cpp
    catch2_unit_test_device_data.cpp
    catch2_w_attribute_set_write_value.cpp
    # These currrently fail on Windows and need investigating
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:catch2_filedatabase.cpp>
    catch2_jpeg_encoding.cpp
    catch2_loggerstream_attribute.cpp
    catch2_device_proxy.cpp
    $<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<STREQUAL:$<TARGET_PROPERTY:tango,TYPE>,SHARED_LIBRARY>>:catch2_create_cpp_class.cpp>
    )
target_link_libraries(Catch2Tests PRIVATE $<$<BOOL:${nlohmann_json_FOUND}>:nlohmann_json::nlohmann_json>)
