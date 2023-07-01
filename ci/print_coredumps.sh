#!/usr/bin/env bash

set -e

src_dir="/home/tango/src/"
dline="================================================================================"

for corefile in build/tests/core.*; do
  if [ -e "$corefile" ]; then
    # extract %e from the core pattern: core.%e.%p.%t
    bin_name=$(basename $corefile | awk 'BEGIN { FS="." } { print $2 }')

    # $bin_name is limited to 15 characters, so we might have truncation compared
    # to the name of the binary in the file system.  In this loop we try dumping a
    # stack trace with each binary that matches.  There is only a single match
    # that will work but, in practice, we will probably only see a single match
    # anyway so it is not worth trying to work out which is the correct one here.

    for bin_path in $(find . -name "${bin_name}*" -type f -executable); do
      echo -e "$dline\nBacktrace for corefile=$corefile (binary=$bin_path)\n$dline"
      docker exec -w "${src_dir}" cpp_tango gdb --batch $bin_path ./$corefile -ex "bt full"
      echo
    done
  fi
done
