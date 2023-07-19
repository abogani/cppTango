#! /usr/bin/env bash

set -e

if [[ -z "$CI_TARGET_BRANCH" ]]
then
  CI_TARGET_BRANCH=main
fi

if [[ -z "$CMAKE_BUILD_PARALLEL_LEVEL" ]]
then
  export CMAKE_BUILD_PARALLEL_LEVEL=$(grep -c ^processor /proc/cpuinfo)
fi

function exit_on_abi_api_breakages() {
  local reports=compat_reports/libtango/${old_revision}_to_${new_revision}
  if [ -e ${reports}/abi_affected.txt ]; then
    echo "ABI breakages detected:"
    cat ${reports}/abi_affected.txt | c++filt
  fi
  if [ -e ${reports}/src_affected.txt ]; then
    echo "API breakages detected:"
    cat ${reports}/src_affected.txt | c++filt
  fi
  exit 1
}

function prepare_old() {
  git worktree remove --force old-branch 2>/dev/null || true
  git worktree add old-branch origin/${CI_TARGET_BRANCH}
}

function prepare_new() {
  local revision=$(git rev-parse HEAD)

  git worktree remove --force new-branch 2>/dev/null || true
  git branch -D ci/abi-api-test-merge 2>/dev/null || true
  git worktree add -b ci/abi-api-test-merge new-branch origin/${CI_TARGET_BRANCH}
  cd new-branch
  git merge ${revision} --no-commit
  cd "${base}"
}

# $1 string prefix
function generate_info() {
  local prefix=$1

  mkdir ${prefix}-branch/build
  cd ${prefix}-branch/build
  cmake                                    \
    -Werror=dev                            \
    -DCMAKE_BUILD_TYPE=Debug               \
    -DTANGO_CPPZMQ_BASE=/home/tango        \
    -DBUILD_TESTING=OFF                    \
    -DCMAKE_DISABLE_PRECOMPILE_HEADERS=OFF \
    -DCMAKE_CXX_FLAGS=-gdwarf-4            \
    ..
  cmake --build .
  abi-dumper libtango.so -o ${base}/libtango-${prefix}.dump -lver ${prefix}
  cd "${base}"
}

base=$(pwd)

prepare_old
prepare_new
generate_info "old"
generate_info "new"

# Compare results
abi-compliance-checker -l libtango -old ${base}/libtango-old.dump \
                                   -new ${base}/libtango-new.dump \
                                   -list-affected || exit_on_abi_api_breakages
