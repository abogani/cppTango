#! /usr/bin/env bash

set -e

# taken from https://stackoverflow.com/a/246128
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source "${SCRIPT_DIR}"/vars.sh

if [[ "$GITLAB_CI" == "true" ]]
then
  echo "Preparing for test run inside CI"
  ulimit -c unlimited
  sudo sh -c 'echo "core.%e.%p.%t" > /proc/sys/kernel/core_pattern'

  docker pull registry.gitlab.com/tango-controls/docker/mysql:5.16-mysql-5
  docker pull registry.gitlab.com/tango-controls/docker/tango-db:5.16-1

  # Setup a route to the containers running inside dind
  sudo ip route add $(docker network inspect -f '{{(index .IPAM.Config 0).Subnet}}' bridge) \
  via                                                                                       \
  $(getent hosts docker | awk '{ print $1 }')                                               \
  dev eth0
fi

ADDITIONAL_ARGS=""

if [[ -n "${SKIP_TESTS}" ]]
then
  ADDITIONAL_ARGS="${ADDITIONAL_ARGS} -E ${SKIP_TESTS}"
fi

# prefer --test-dir once cmake 3.20 is mandatory, see
# https://stackoverflow.com/a/66280917

# restore CWD on script exit
STORED_CWD=${PWD}
trap "cd ${STORED_CWD}" EXIT

cd ${BUILD_DIR}
ctest --output-on-failure ${ADDITIONAL_ARGS}
