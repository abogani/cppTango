#! /usr/bin/env bash

set -e

# taken from https://stackoverflow.com/a/246128
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source "${SCRIPT_DIR}"/vars.sh

dline="================================================================================"

for corefile in build/tests/core.*; do
  if [ -e "$corefile" ]; then
    gdb --cd=${SOURCE_DIR} --batch -core ./$corefile -ex "thread apply all bt full"
  fi
done
