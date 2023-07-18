#!/usr/bin/env bash

set -e

if [[ $RUN_TESTS == "OFF" ]]
then
  echo "Skipping tests as requested"
  exit 0
fi

build_dir="/home/tango/src/build"

function run_in_container {
  docker exec \
    -w "${build_dir}" \
    -e CTEST_OUTPUT_ON_FAILURE=ON \
    cpp_tango "$@"
}

run_in_container ctest
