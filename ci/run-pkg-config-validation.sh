#!/usr/bin/env bash

set -e

echo "############################"
echo "CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
echo "OS_TYPE=$OS_TYPE"
echo "TANGO_HOST=$TANGO_HOST"
echo "CMAKE_DISABLE_PRECOMPILE_HEADERS=$CMAKE_DISABLE_PRECOMPILE_HEADERS"
echo "BUILD_SHARED_LIBS=$BUILD_SHARED_LIBS"
echo "TANGO_USE_LIBCPP=$TANGO_USE_LIBCPP"
echo "TANGO_WARNINGS_AS_ERRORS=$TANGO_WARNINGS_AS_ERRORS"
echo "TOOLCHAIN_FILE=$TOOLCHAIN_FILE"
echo "############################"

docker exec cpp_tango mkdir -p /home/tango/src/build

# set defaults
MAKEFLAGS=${MAKEFLAGS:- -j $(nproc)}
TANGO_USE_LIBCPP=${TANGO_USE_LIBCPP:-OFF}
CMAKE_DISABLE_PRECOMPILE_HEADERS=${CMAKE_DISABLE_PRECOMPILE_HEADERS:-ON}
BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS:-ON}
TANGO_WARNINGS_AS_ERRORS=${TANGO_WARNINGS_AS_ERRORS:-OFF}
SOURCE_DIR=/home/tango/src
BUILD_DIR=${SOURCE_DIR}/build

ADDITIONAL_ARGS=""

if [[ -f "$TOOLCHAIN_FILE" && -s "$TOOLCHAIN_FILE" ]]
then
  ADDITIONAL_ARGS="${ADDITIONAL_ARGS} -DCMAKE_TOOLCHAIN_FILE=${SOURCE_DIR}/${TOOLCHAIN_FILE}"
fi

# NOTE: Below we are lying to cmake about the OMNIORB_VERSION because the
# version provided by Fedora is unsupported, however, for this test we do not
# want to run anything, so we don't care.  If you are going to copy this for
# some other CI job using Fedora, please set STOCK_OMNIORB=OFF and remove the
# -DOMNIORB_VERSION
docker exec cpp_tango cmake                                                \
  -Werror=dev                                                              \
  -H${SOURCE_DIR}                                                          \
  -B${BUILD_DIR}                                                           \
  -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}                                 \
  -DCMAKE_VERBOSE_MAKEFILE=ON                                              \
  -DTANGO_CPPZMQ_BASE=/home/tango                                          \
  -DTANGO_IDL_BASE=/home/tango                                             \
  -DOMNIORB_VERSION="4.2.5"                                                \
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}                                   \
  -DCMAKE_DISABLE_PRECOMPILE_HEADERS=${CMAKE_DISABLE_PRECOMPILE_HEADERS}   \
  -DTANGO_USE_JPEG=${TANGO_USE_JPEG}                                       \
  -DTANGO_USE_LIBCPP=${TANGO_USE_LIBCPP}                                   \
  -DTANGO_WARNINGS_AS_ERRORS=${TANGO_WARNINGS_AS_ERRORS}                   \
  -DTANGO_ENABLE_COVERAGE=${TANGO_ENABLE_COVERAGE:-OFF}                    \
  ${ADDITIONAL_ARGS}

docker exec cpp_tango mkdir -p ${BUILD_DIR}/tests/results

docker exec cpp_tango bash -c "PKG_CONFIG_PATH=/home/tango/lib/pkgconfig pkg-config --validate ${BUILD_DIR}/tango.pc 2>&1 | tee ${BUILD_DIR}/tests/results/pkgconfig-validation.log"

if [[ -f "build/tests/results/pkgconfig-validation.log" ]] 
then
	if [[ -s "build/tests/results/pkgconfig-validation.log" ]] 
	then
		# The log file is not empty. Some errors got reported during pkg-config validation
		echo "ERROR: tango.pc file pkg-config validation NOT OK"
		exit 1
	else
		echo "SUCCESS: tango.pc file pkg-config validation OK"
		exit 0
	fi
else
	echo "ERROR: Could not create ${BUILD_DIR}/tests/results/pkgconfig-validation.log file"
	exit 2
fi
