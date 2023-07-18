#!/usr/bin/env bash

set -e

SOURCE_DIR=.
BUILD_DIR=${SOURCE_DIR}/build

mkdir -p ${BUILD_DIR}/tests/results

pkg-config --validate ${BUILD_DIR}/tango.pc 2>&1 | tee ${BUILD_DIR}/tests/results/pkgconfig-validation.log

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
