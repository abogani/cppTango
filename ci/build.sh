#! /usr/bin/env bash

set -e

# taken from https://stackoverflow.com/a/246128
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source "${SCRIPT_DIR}"/vars.sh

MAKEFLAGS=${MAKEFLAGS:- -j $(nproc)}
TANGO_MAKE_TARGET=${TANGO_MAKE_TARGET:-all}

make -C ${BUILD_DIR} ${MAKEFLAGS} ${TANGO_MAKE_TARGET}
