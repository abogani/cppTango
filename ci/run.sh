#!/usr/bin/env bash

set -e

echo "############################"
echo "CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
echo "OS_TYPE=$OS_TYPE"
echo "TANGO_HOST=$TANGO_HOST"
echo "TANGO_USE_PCH=$TANGO_USE_PCH"
echo "BUILD_SHARED_LIBS=$BUILD_SHARED_LIBS"
echo "TANGO_USE_LIBCPP=$TANGO_USE_LIBCPP"
echo "TANGO_WARNINGS_AS_ERRORS=$TANGO_WARNINGS_AS_ERRORS"
echo "TOOLCHAIN_FILE=$TOOLCHAIN_FILE"
echo "############################"

docker exec cpp_tango mkdir -p /home/tango/src/build

# set defaults
MAKEFLAGS=${MAKEFLAGS:- -j $(nproc)}
TANGO_USE_LIBCPP=${TANGO_USE_LIBCPP:-OFF}
TANGO_USE_PCH=${TANGO_USE_PCH:-OFF}
BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS:-ON}
TANGO_WARNINGS_AS_ERRORS=${TANGO_WARNINGS_AS_ERRORS:-OFF}
SOURCE_DIR=/home/tango/src
BUILD_DIR=${SOURCE_DIR}/build

ADDITIONAL_ARGS=""

if [[ -f "$TOOLCHAIN_FILE" && -s "$TOOLCHAIN_FILE" ]]
then
  ADDITIONAL_ARGS="${ADDITIONAL_ARGS} -DCMAKE_TOOLCHAIN_FILE=${SOURCE_DIR}/${TOOLCHAIN_FILE}"
fi

docker exec cpp_tango cmake                                \
  -H${SOURCE_DIR}                                          \
  -B${BUILD_DIR}                                           \
  -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}                 \
  -DCMAKE_VERBOSE_MAKEFILE=ON                              \
  -DTANGO_CPPZMQ_BASE=/home/tango                          \
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}                   \
  -DTANGO_USE_PCH=${TANGO_USE_PCH}                         \
  -DTANGO_USE_JPEG=${TANGO_USE_JPEG}                       \
  -DTANGO_USE_LIBCPP=${TANGO_USE_LIBCPP}                   \
  -DTANGO_WARNINGS_AS_ERRORS=${TANGO_WARNINGS_AS_ERRORS}   \
  -DTANGO_ENABLE_COVERAGE=${TANGO_ENABLE_COVERAGE:-OFF}    \
  ${ADDITIONAL_ARGS}

docker exec cpp_tango make -C ${BUILD_DIR} $MAKEFLAGS
