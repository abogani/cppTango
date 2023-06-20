#!/usr/bin/env bash
# vim: syntax=sh

export SHELLOPTS
set -o errexit
set -o nounset

pids=$(tr '\n' ' ' <"${TANGO_TEST_CASE_DIRECTORY}/server_pids")

if [[ -z "$pids" ]]; then
    echo "No device servers to kill"
    exit
fi

echo "Killing PIDs: $pids"
kill -s TERM --timeout 5000 KILL -- $pids &>/dev/null
