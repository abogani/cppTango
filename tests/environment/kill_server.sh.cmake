#! /usr/bin/env bash
# vim: syntax=sh

export SHELLOPTS
set -o errexit
set -o nounset

export pids=$(tr '\n' ' ' <"${TANGO_TEST_CASE_DIRECTORY}/server_pids")

function wait_pids() {
    local dead=
    while [[ $(echo $pids | wc -w) -gt $(echo $dead | wc -w) ]]; do
        dead=
        for pid in $pids; do
            if ! ps --pid $pid &>/dev/null; then
                dead="$dead $pid"
            fi
        done
        sleep 0.1s
    done
}
export -f wait_pids

if [[ -z "$pids" ]]; then
    echo "No device servers to kill"
    exit
fi

echo "Killing PIDs: $pids"
kill -s TERM $pids
timeout 5s bash -c wait_pids
if [[ $? -eq 124 ]]; then
    kill -s KILL $pids
fi
