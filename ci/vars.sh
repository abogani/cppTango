#! /usr/bin/env bash

set -e

function ncore()
{
    os=$(uname -s)
    case ${os} in
        Linux) ncpu=$(nproc);;
        Darwin) ncpu=$(sysctl -n hw.logicalcpu);;
        *) ncpu=-1;;
    esac

    echo "${ncpu}"
}

SOURCE_DIR=.
BUILD_DIR=${SOURCE_DIR}/build
INSTALL_DIR=/home/tango
NUM_PROCESSORS=$(ncore)

unset ncore
