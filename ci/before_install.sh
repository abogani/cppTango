#!/usr/bin/env bash

set -e

if [[ "$RUN_TESTS" == "ON" ]]
then
  docker pull registry.gitlab.com/tango-controls/docker/mysql:5.16-mysql-5
  docker pull registry.gitlab.com/tango-controls/docker/tango-db:5.16
fi

if [[ "$STOCK_CPPZMQ" == "OFF" ]]
then
  git clone -b v4.7.1 --depth 1 https://github.com/zeromq/cppzmq.git
else
  mkdir cppzmq
fi

mkdir omniorb
if [[ "$STOCK_OMNIORB" == "OFF" ]]
then
    apt-get install bzip2
    wget -OomniORB.tar.bz2 https://sourceforge.net/projects/omniorb/files/omniORB/omniORB-4.2.5/omniORB-4.2.5.tar.bz2/download
    tar xaf omniORB.tar.bz2 -Comniorb/ --strip-components=1
fi

git clone -b $TANGO_IDL_TAG --depth 1 https://gitlab.com/tango-controls/tango-idl.git idl
