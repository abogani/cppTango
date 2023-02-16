#!/usr/bin/env bash

echo "Configure omniORB"
docker exec cpp_tango sh -c "cd /home/tango/omniorb/ && ./configure --prefix=/home/tango/"
if [ $? -ne "0" ]
then
    exit -1
fi
echo "Install omniORB"
docker exec cpp_tango make -C /home/tango/omniorb/ -j$(nproc) all install
if [ $? -ne "0" ]
then
    exit -1
fi
