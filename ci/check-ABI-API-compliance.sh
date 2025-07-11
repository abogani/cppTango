#! /usr/bin/env bash

set -e

# taken from https://stackoverflow.com/a/246128
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source "${SCRIPT_DIR}"/vars.sh

if [[ -z "$CI_TARGET_BRANCH" ]]
then
  CI_TARGET_BRANCH=main
fi

function exit_on_abi_api_breakages() {
  local reports=compat_reports/libtango/old_to_new
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
  cd old-branch
  git submodule update --init
  cd "${base}"
}

function prepare_new() {
  local revision=$(git rev-parse HEAD)

  git worktree remove --force new-branch 2>/dev/null || true
  git branch -D ci/abi-api-test-merge 2>/dev/null || true
  git worktree add -b ci/abi-api-test-merge new-branch origin/${CI_TARGET_BRANCH}
  cd new-branch
  # We initialise submodules after the merge in case they are updated in the MR
  # branch.
  git merge ${revision} --no-commit
  git submodule update --init
  cd "${base}"
}

# $1 string prefix
function generate_info() {
  local prefix=$1

  mkdir ${prefix}-branch/build
  cd ${prefix}-branch/build
  cmake                                                 \
    -Werror=dev                                         \
    -DCMAKE_BUILD_TYPE=Debug                            \
    -Dcppzmq_ROOT=/home/tango                           \
    -DBUILD_TESTING=OFF                                 \
    -DCMAKE_DISABLE_PRECOMPILE_HEADERS=OFF              \
    -DTANGO_USE_TELEMETRY="${TANGO_USE_TELEMETRY:-OFF}" \
    -DCMAKE_CXX_FLAGS=-gdwarf-4                         \
    ..
  cmake --build . --parallel ${NUM_PROCESSORS}
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
