#! /usr/bin/env bash
# vim: syntax=sh

set -e

export TANGO_TEST_CASE_DIRECTORY="${TANGO_TEST_CASE_DIRECTORY:-.}"

"@PROJECT_BINARY_DIR@/tests/conf_devtest" \
    @SERV_NAME@/@INST_NAME@ \
    @SERV_NAME@/@INST_NAME2@ \
    @FWD_SERV_NAME@/@INST_NAME@ \
    @DEV1@ \
    @DEV2@ \
    @DEV3@ \
    @DEV1_ALIAS@ \
    @ATTR_ALIAS@ \
    @FWD_DEV@ \
    @DEV20@ \
    &> "${TANGO_TEST_CASE_DIRECTORY}/conf_devtest.log"

# Bug reported in cppTango#1022 occurs when there are several devices defined for the same device server instance
# We define 2 devices:
"@CMAKE_CURRENT_BINARY_DIR@/tango_admin.sh" --add-server TestCppTango1022/@INST_NAME@ TestCppTango1022 @TEST_CPPTANGO_1022_DEV1@,@TEST_CPPTANGO_1022_DEV2@

"@CMAKE_CURRENT_BINARY_DIR@/start_server.sh" @INST_NAME@ DevTest device_server/generic

"@CMAKE_CURRENT_BINARY_DIR@/start_server.sh" @INST_NAME@ FwdTest device_server/forward

"@CMAKE_CURRENT_BINARY_DIR@/start_server.sh" @INST_NAME@ TestCppTango1022 device_server/TestCppTango1022
