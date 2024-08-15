find_package (Threads REQUIRED)

find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_FOUND)
  find_program(Python3_EXECUTABLE NAMES python3)
endif()

if(NOT Python3_EXECUTABLE)
  message(FATAL_ERROR "Could not find python 3.")
endif()

#TODO different OS
set(SERV_NAME "DevTest")
set(FWD_SERV_NAME "FwdTest")
set(INST_NAME "test")
set(INST_NAME2 "test2")
set(DEV1 "${INST_NAME}/debian8/10")
set(DEV2 "${INST_NAME}/debian8/11")
set(DEV3 "${INST_NAME}/debian8/12")
set(DEV20 "${INST_NAME}2/debian8/20")
set(FWD_DEV "${INST_NAME}/fwd_debian8/10")
set(TEST_CPPTANGO_1022_DEV1 "${INST_NAME}/cpptango-1022/1")
set(TEST_CPPTANGO_1022_DEV2 "${INST_NAME}/cpptango-1022/2")
set(TEST_CPPTANGO_1022_SERV_NAME "TestCppTango1022")
set(DEV1_ALIAS "debian8_alias")
set(ATTR_ALIAS "debian8_attr_alias")
set(COMMON_TEST_DEFS _REENTRANT)
cmake_host_system_information(RESULT HOST_NAME QUERY FQDN)
string(TOLOWER ${HOST_NAME} HOST_NAME)
message("HOST_NAME=${HOST_NAME}")

add_subdirectory(environment)

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

function(tango_add_test)
    cmake_parse_arguments(TT "" "NAME;COMMAND" "IDL6;IDL5" ${ARGN})

    add_test(
        NAME "${TT_NAME}"
        COMMAND "${CMAKE_CURRENT_BINARY_DIR}/environment/run_with_fixture.sh" "${TT_COMMAND}"
        ${TT_UNPARSED_ARGUMENTS} ${TT_IDL6})

    add_test(
        NAME "idl5::${TT_NAME}"
        COMMAND "env" "IDL_SUFFIX=_IDL5" "${CMAKE_CURRENT_BINARY_DIR}/environment/run_with_fixture.sh" "${TT_COMMAND}"
        ${TT_UNPARSED_ARGUMENTS} ${TT_IDL5})
endfunction()

add_executable(conf_devtest conf_devtest.cpp)
target_compile_definitions(conf_devtest PRIVATE ${COMMON_TEST_DEFS})
target_link_libraries(conf_devtest PUBLIC Tango::Tango ${CMAKE_DL_LIBS})
target_precompile_headers(conf_devtest PRIVATE ${TANGO_SOURCE_DIR}/src/include/tango/tango.h)
add_library(compare_test_object OBJECT compare_test.cpp compare_test.h cxx_common.h)
target_compile_definitions(compare_test_object PRIVATE ${COMMON_TEST_DEFS} ${OMNIORB_PKG_CFLAGS_OTHER})

add_subdirectory(device_server)
add_subdirectory(cxxtest)

function(CXX_GENERATE_TEST_EXEC name)

    set(INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp")

    add_custom_command(OUTPUT ${name}.cpp
                       COMMAND "${Python3_EXECUTABLE}" ${CMAKE_CURRENT_SOURCE_DIR}/cxxtest/bin/cxxtestgen.py
                               --template=${CMAKE_CURRENT_SOURCE_DIR}/cxxtest/template/tango_template.tpl
                               -o ${name}.cpp
                               ${INPUT_FILE}
                       DEPENDS Tango::Tango
                               ${INPUT_FILE}
                               ${CMAKE_CURRENT_SOURCE_DIR}/cxxtest/template/tango_template.tpl
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       COMMENT "Generate ${name}.cpp")

    add_executable(${name} $<TARGET_OBJECTS:compare_test_object> ${name}.cpp
        common.cpp)
    target_include_directories(${name} PRIVATE cxxtest/include ${CMAKE_CURRENT_BINARY_DIR}/cxxtest/include)
    target_link_libraries(${name} PRIVATE Tango::Tango Threads::Threads)
    target_compile_definitions(${name} PRIVATE ${COMMON_TEST_DEFS})
    set_cflags_and_include(${name})

    target_precompile_headers(${name} REUSE_FROM conf_devtest)
endfunction()

# Pass TRUE as second argument if your tests don't require the device servers
function(CXX_GENERATE_TEST name)

    if(${ARGC} EQUAL 2)
        if(ARGV1)
            set(SKIP_FIXTURES "--skip-fixtures")
        endif()
    endif()

    CXX_GENERATE_TEST_EXEC(${name})

    tango_add_test(NAME "CXX::${name}" COMMAND $<TARGET_FILE:${name}>
            ${SKIP_FIXTURES}
            --device1=${DEV1}
            --device2=${DEV2}
            --device3=${DEV3}
            --device20=${DEV20}
            --fwd_device=${FWD_DEV}
            --loop=1
            --serverhost=${HOST_NAME}
            --clienthost=${HOST_NAME}
            --serverversion=6
            --docurl=http://www.tango-controls.org
            --devtype=TestDevice
            --dbserver=sys/database/2
            --outpath=/tmp/
            --refpath=${CMAKE_CURRENT_SOURCE_DIR}/resources/
            --loglevel=0
            --dsloglevel=5
            --suiteloop=1
            --devicealias=${DEV1_ALIAS}
            --attributealias=${ATTR_ALIAS}
            IDL6 --fulldsname=${SERV_NAME}/${INST_NAME}
            IDL5 --fulldsname=${SERV_NAME}_IDL5/${INST_NAME})
endfunction()

option(TANGO_SKIP_OLD_TESTS "Skips building the old tests" OFF)

if (NOT TANGO_SKIP_OLD_TESTS)
    CXX_GENERATE_TEST(cxx_always_hook)
    CXX_GENERATE_TEST(cxx_asyn_reconnection)
    CXX_GENERATE_TEST(cxx_attr)
    CXX_GENERATE_TEST(cxx_attr_conf)
    CXX_GENERATE_TEST(cxx_attr_misc)
    CXX_GENERATE_TEST(cxx_attr_write)
    CXX_GENERATE_TEST(cxx_attrprop)
    CXX_GENERATE_TEST(cxx_blackbox)
    CXX_GENERATE_TEST(cxx_class_dev_signal)
    CXX_GENERATE_TEST(cxx_class_signal)
    CXX_GENERATE_TEST(cxx_cmd_query)
    CXX_GENERATE_TEST(cxx_cmd_types)
    CXX_GENERATE_TEST(cxx_common_net TRUE)
    CXX_GENERATE_TEST(cxx_database)
    CXX_GENERATE_TEST(cxx_dev_intr_event_not_running)
    CXX_GENERATE_TEST(cxx_device_pipe_blob TRUE)
    CXX_GENERATE_TEST(cxx_dserver_cmd)
    CXX_GENERATE_TEST(cxx_dserver_misc)
    CXX_GENERATE_TEST_EXEC(cxx_dynamic_attributes)
    tango_add_test(NAME "CXX::cxx_dynamic_attributes" COMMAND $<TARGET_FILE:cxx_dynamic_attributes>
                --device1=${TEST_CPPTANGO_1022_DEV1}
                --loop=1
                IDL6 --fulldsname=${TEST_CPPTANGO_1022_SERV_NAME}/${INST_NAME}
                IDL5 --fulldsname=${TEST_CPPTANGO_1022_SERV_NAME}_IDL5/${INST_NAME})
    CXX_GENERATE_TEST(cxx_encoded)
    CXX_GENERATE_TEST(cxx_enum_att)
    CXX_GENERATE_TEST(cxx_exception)
    CXX_GENERATE_TEST(cxx_fwd_att)
    CXX_GENERATE_TEST(cxx_group)
    CXX_GENERATE_TEST(cxx_mem_attr)
    CXX_GENERATE_TEST(cxx_misc)
    CXX_GENERATE_TEST(cxx_misc_util)
    CXX_GENERATE_TEST(cxx_nan_inf_in_prop)
    CXX_GENERATE_TEST(cxx_old_poll)
    CXX_GENERATE_TEST(cxx_pipe)
    CXX_GENERATE_TEST(cxx_pipe_conf)
    CXX_GENERATE_TEST(cxx_poll)
    CXX_GENERATE_TEST(cxx_poll_admin)
    CXX_GENERATE_TEST(cxx_reconnection_zmq)
    CXX_GENERATE_TEST(cxx_seq_vec)
    CXX_GENERATE_TEST(cxx_server_configure_zmq_ports)
    CXX_GENERATE_TEST(cxx_server_event)
    CXX_GENERATE_TEST(cxx_signal)#TODO Windows
    CXX_GENERATE_TEST(cxx_stateless_subscription)
    CXX_GENERATE_TEST(cxx_syntax)
    CXX_GENERATE_TEST(cxx_templ_cmd)
    CXX_GENERATE_TEST(cxx_test_state_on)
    CXX_GENERATE_TEST(cxx_write_attr_hard)
    CXX_GENERATE_TEST(cxx_z00_dyn_cmd)
endif()

# Failing tests
# CXX_GENERATE_TEST(cxx_zmcast01_simple)
# CXX_GENERATE_TEST(cxx_zmcast02_local_remote)
# CXX_GENERATE_TEST(cxx_zmcast03_svr_local_remote)

set(TESTS asyn_attr_cb
          asyn_attr
          asyn_attr_multi
          asyn_cb2
          asyn_cb_cmd
          asyn_cb
          asyn_cmd
          asyn_faf
          asyn_thread
          asyn_write_attr
          asyn_write_attr_multi
          asyn_write_cb
          auto_asyn_cmd
          archive_event
          att_conf_event
          att_conf_event_buffer
          att_type_event
          back_per_event
          change_event
          change_event64
          change_event_buffer
          data_ready_event
          data_ready_event_buffer
          dev_intr_event
          event_lock
          multi_dev_event
          multi_event
          per_event
          pipe_event
          state_event
          user_event
          acc_right
          add_rem_attr
          add_rem_dev
          allowed_cmd
          att_conf
          attr_conf_test
          attr_misc
          attr_proxy
          attr_types
          cmd_types
          ConfEventBugClient
          copy_devproxy
          ds_cache
          device_proxy_properties
          helper
          lock
          locked_device
          mem_att
          misc_devattr
          misc_devdata
          misc_devproxy
          multi_thread_sighandler # FIXME: Windows
          new_devproxy
          obj_prop
          poll_except
          print_data
          print_data_hist
          prop_list
          rds
          read_hist_ext
          reconnect_attr
          reconnect
          restart_device
          ring_depth
          state_attr
          sub_dev
          unlock
          w_r_attr
          write_attr_3
          write_attr)

# Failing tests:
# scan, back_ch_event, back_per_event, locked_device

function(TEST_SUITE_ADD_TEST test)
    add_executable(${test} ${test}.cpp common.cpp)
    target_compile_definitions(${test} PUBLIC ${COMMON_TEST_DEFS})
    target_link_libraries(${test} Tango::Tango ${CMAKE_DL_LIBS} Threads::Threads)
    target_precompile_headers(${test} REUSE_FROM conf_devtest)
        #TODO generalize tests
#    add_test(NAME "CPP::${test}"  COMMAND $<TARGET_FILE:${test}> ${DEV1} ${DEV2} ${DEV3} ${DEV1_ALIAS})
endfunction()

if (NOT TANGO_SKIP_OLD_TESTS)
    foreach(TEST ${TESTS})
        TEST_SUITE_ADD_TEST(${TEST})
    endforeach(TEST)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      set_source_files_properties(
            cxx_encoded.cpp cmd_types.cpp helper.cpp
            PROPERTIES
            COMPILE_FLAGS -Wno-stringop-overflow)
    endif()

    configure_file(locked_device_cmd.h.cmake locked_device_cmd.h @ONLY)
    target_include_directories(lock PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()

include(catch2/tango_catch2_tests.cmake)

tango_catch2_tests_create(
    catch2_alarm_event.cpp
    catch2_alarm.cpp
    catch2_attr_manip.cpp
    catch2_attr_polling.cpp
    catch2_cmd_polling.cpp
    catch2_test_dtypes.cpp
    catch2_dev_state.cpp
    catch2_internal_utils.cpp
    catch2_nodb_connection.cpp
    catch2_change_event_on_nan.cpp
    $<$<BOOL:${TANGO_USE_TELEMETRY}>:${CMAKE_CURRENT_SOURCE_DIR}/catch2_telemetry.cpp>
    catch2_tango_monitor.cpp
    catch2_w_attribute_set_write_value.cpp
    # These currrently fail on Windows and need investigating
    $<$<NOT:$<BOOL:WIN32>>:catch2_filedatabase.cpp>
    $<$<NOT:$<BOOL:WIN32>>:catch2_jpeg_encoding.cpp>
    )

if (NOT TANGO_SKIP_OLD_TESTS)
    tango_add_test(NAME "asyn::asyn_cmd"  COMMAND $<TARGET_FILE:asyn_cmd> ${DEV1})
    tango_add_test(NAME "asyn::asyn_attr"  COMMAND $<TARGET_FILE:asyn_attr> ${DEV1})
    tango_add_test(NAME "asyn::asyn_attr_multi"  COMMAND $<TARGET_FILE:asyn_attr_multi> ${DEV1})
    tango_add_test(NAME "asyn::asyn_write_attr"  COMMAND $<TARGET_FILE:asyn_write_attr> ${DEV1})
    tango_add_test(NAME "asyn::asyn_write_attr_multi"  COMMAND $<TARGET_FILE:asyn_write_attr_multi> ${DEV1})
    tango_add_test(NAME "asyn::asyn_cb"  COMMAND $<TARGET_FILE:asyn_cb> ${DEV1} ${DEV2})
    tango_add_test(NAME "asyn::asyn_cb2"  COMMAND $<TARGET_FILE:asyn_cb2> ${DEV1} ${DEV2})
    tango_add_test(NAME "asyn::asyn_cb_cmd"  COMMAND $<TARGET_FILE:asyn_cb_cmd> ${DEV1})
    tango_add_test(NAME "asyn::asyn_attr_cb"  COMMAND $<TARGET_FILE:asyn_attr_cb> ${DEV1})
    tango_add_test(NAME "asyn::asyn_write_cb"  COMMAND $<TARGET_FILE:asyn_write_cb> ${DEV1})
    tango_add_test(NAME "asyn::auto_asyn_cmd"  COMMAND $<TARGET_FILE:auto_asyn_cmd> ${DEV1})

    tango_add_test(NAME "event::archive_event"  COMMAND $<TARGET_FILE:archive_event> ${DEV1})
    tango_add_test(NAME "event::att_conf_event"  COMMAND $<TARGET_FILE:att_conf_event> ${DEV1})
    tango_add_test(NAME "event::att_conf_event_buffer"  COMMAND $<TARGET_FILE:att_conf_event_buffer> ${DEV1})
    tango_add_test(NAME "event::att_type_event"  COMMAND $<TARGET_FILE:att_type_event> ${DEV1})
    tango_add_test(NAME "event::change_event"  COMMAND $<TARGET_FILE:change_event> ${DEV1})
    tango_add_test(NAME "event::change_event64"  COMMAND $<TARGET_FILE:change_event64> ${DEV1})
    tango_add_test(NAME "event::change_event_buffer"  COMMAND $<TARGET_FILE:change_event_buffer> ${DEV1})
    tango_add_test(NAME "event::data_ready_event"  COMMAND $<TARGET_FILE:data_ready_event> ${DEV1})
    tango_add_test(NAME "event::data_ready_event_buffer"  COMMAND $<TARGET_FILE:data_ready_event_buffer> ${DEV1})
    tango_add_test(NAME "event::dev_intr_event"  COMMAND $<TARGET_FILE:dev_intr_event> ${DEV1})
    tango_add_test(NAME "event::event_lock"  COMMAND $<TARGET_FILE:event_lock> ${DEV1})
    tango_add_test(NAME "event::multi_dev_event"  COMMAND $<TARGET_FILE:multi_dev_event> ${DEV1} ${DEV2} ${DEV3})
    tango_add_test(NAME "event::multi_event"  COMMAND $<TARGET_FILE:multi_event> ${DEV1})
    tango_add_test(NAME "event::per_event"  COMMAND $<TARGET_FILE:per_event> ${DEV1} ${DEV2})
    tango_add_test(NAME "event::pipe_event"  COMMAND $<TARGET_FILE:pipe_event> ${DEV1})
    tango_add_test(NAME "event::state_event"  COMMAND $<TARGET_FILE:state_event> ${DEV1})
    tango_add_test(NAME "event::user_event"  COMMAND $<TARGET_FILE:user_event> ${DEV1})

    tango_add_test(NAME "old_tests::ConfEventBugClient"  COMMAND $<TARGET_FILE:ConfEventBugClient> ${DEV3})
    tango_add_test(NAME "old_tests::att_conf"  COMMAND $<TARGET_FILE:att_conf> ${DEV1})
    tango_add_test(NAME "old_tests::attr_conf_test"  COMMAND $<TARGET_FILE:attr_conf_test> ${DEV1})
    tango_add_test(NAME "old_tests::multi_thread_sighandler"  COMMAND $<TARGET_FILE:multi_thread_sighandler>)
    tango_add_test(NAME "old_tests::attr_misc"  COMMAND $<TARGET_FILE:attr_misc> ${DEV1})
    tango_add_test(NAME "old_tests::attr_proxy"  COMMAND $<TARGET_FILE:attr_proxy> ${DEV1}/Short_attr_rw)
    tango_add_test(NAME "old_tests::attr_types"  COMMAND $<TARGET_FILE:attr_types> ${DEV1} 10)
    tango_add_test(NAME "old_tests::cmd_types"  COMMAND $<TARGET_FILE:cmd_types> ${DEV1} 10)
    tango_add_test(NAME "old_tests::copy_devproxy"  COMMAND $<TARGET_FILE:copy_devproxy> ${DEV1} ${DEV2} ${DEV3} IDL5 5 IDL6 6)
    tango_add_test(NAME "old_tests::ds_cache"  COMMAND $<TARGET_FILE:ds_cache>)
    tango_add_test(NAME "old_tests::device_proxy_properties"  COMMAND $<TARGET_FILE:device_proxy_properties> ${DEV1})
    tango_add_test(NAME "old_tests::lock"  COMMAND $<TARGET_FILE:lock> ${DEV1} ${DEV2})
    tango_add_test(NAME "old_tests::mem_att"  COMMAND $<TARGET_FILE:mem_att> ${DEV1})
    tango_add_test(NAME "old_tests::misc_devattr"  COMMAND $<TARGET_FILE:misc_devattr>)
    tango_add_test(NAME "old_tests::misc_devdata"  COMMAND $<TARGET_FILE:misc_devdata>)
    tango_add_test(NAME "old_tests::misc_devproxy"  COMMAND $<TARGET_FILE:misc_devproxy> ${DEV1} IDL5 ${SERV_NAME}_IDL5/${INST_NAME} 5 IDL6 ${SERV_NAME}/${INST_NAME} 6)
    tango_add_test(NAME "old_tests::obj_prop"  COMMAND $<TARGET_FILE:obj_prop>)
    tango_add_test(NAME "old_tests::print_data"  COMMAND $<TARGET_FILE:print_data> ${DEV1})
    tango_add_test(NAME "old_tests::rds"  COMMAND $<TARGET_FILE:rds> ${DEV1})
    tango_add_test(NAME "old_tests::read_hist_ext"  COMMAND $<TARGET_FILE:read_hist_ext> ${DEV1})
    tango_add_test(NAME "old_tests::ring_depth"  COMMAND $<TARGET_FILE:ring_depth> ${DEV1})
    tango_add_test(NAME "old_tests::state_attr"  COMMAND $<TARGET_FILE:state_attr> ${DEV1})
    tango_add_test(NAME "old_tests::sub_dev"  COMMAND $<TARGET_FILE:sub_dev> ${DEV1} ${DEV2} ${DEV3})
    tango_add_test(NAME "old_tests::w_r_attr"  COMMAND $<TARGET_FILE:w_r_attr> ${DEV1})
    tango_add_test(NAME "old_tests::write_attr"  COMMAND $<TARGET_FILE:write_attr> ${DEV1} 10)
    tango_add_test(NAME "old_tests::write_attr_3"  COMMAND $<TARGET_FILE:write_attr_3> ${DEV1} 10)
endif()
