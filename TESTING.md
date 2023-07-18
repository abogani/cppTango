This document is targeted at cppTango developers only.

## General considerations

We have a large variety of distributions, and OS's, to check cppTango against.
In order to not create a mess in CI in the following we have listed a few
general ideas of how the tests are, and should be, organized:

- Each job MUST use one of the predefined templates:
  - `job-template-with-tests` if you want to run the tests or
  - `job-template-no-test-run` if you do not want to run the tests.

  If you have been longer with us, the variable `RUN_TESTS` has been replaced by this
  job template selection.
- Each job MUST inherit the global list of variables via `<<: *variables`
- Each job MUST use one of the predefined docker images, see [here]( https://gitlab.com/tango-controls/docker/ci/cpptango).
  If none of the available ones fit your purpose, check with the rest of the
  developers if one should either extend an existing one or create a new one.
  Best place to discuss this, is an issue.
- If you have one of the following commands in your `.gitlab-ci.yml`, then this
  indicates bad practice.  Try to avoid this under all circumstances!
  - `docker exec`: Should not be needed because you start already with the correct image
  - `cmake`: Configuration should always be done via `ci/config.sh`
    (`cmake-with-locations-and-min-versions-XXX` is an exception as it does test cmake configuration specialities.)
  - `make`: Building should always be done via `ci/build.sh`
  - `git clone`: The docker image should already include all required dependencies
  - `apt-get`/`dnf`/`apk`: The docker image should already have all required
    packages installed
- All docker images use a non-root user `tango` as default user, `sudo` is
  available in all images which also have docker installed and are able to run the tests.

### Useful scripts in CI and their supported environment variables

- `ci/build.sh`:

   - `MAKEFLAGS`, see `man make`, defaults to `-j <numProcessors>`
   - `TANGO_MAKE_TARGET`, target to build, defaults to `all`, a list of
     available targets can be outputted via `cmake --build . --target help`
     (only available when using the makefile generator)

- `ci/check-abi-api-compliance-check.sh`:

   - `CI_TARGET_BRANCH`, defaults to `main`

- `ci/config.sh`:

   This script accepts all cmake options listed in [Install.md](./Install.md)
   as environment variables and will pass them to cmake.

   Additionally supported:
     - `CMAKE_EXPORT_COMPILE_COMMANDS`, defaults to `off`
     - `CMAKE_UNITY_BUILD`, defaults to `off`
     - `CMAKE_UNITY_BUILD_BATCH_SIZE`, defaults to `0`
     - `OMNIORB_VERSION` for overriding the version check

- `ci/print_coredumps.sh`:

   - None

- `ci/test.sh`:

   - `GITLAB_CI`, set by gitlab CI only to enable specific tweaks, don't set yourself
   - `SKIP_TESTS`, regular expression passed to `ctest -E` for tests to skip

### Updating the CI docker images

- Check with the other developers if this is the right approach
- Change the docker images, see
  [here](https://gitlab.com/tango-controls/docker/ci/cpptango/super) for the
  super project with all of them
- Make a merge request (MR1) on the docker image project in
  https://gitlab.com/tango-controls/docker/ci/cpptango. The CI will build a
  docker image and push that to the registry. It is named like the branch.
- Make a merge request (MR2) against cppTango and use the branch of MR1
- Once review has passed on MR1 and MR2, get MR1 merged and tag the repo
- Update MR2 to point to the newly created tag
- Pass review again and get MR2 merged

### Running the tests

Running the tests requires docker. See
[here](https://docs.docker.com/engine/install) for install instructions.

Once you have installed docker, you can run the tests from `build/` via:

```bash
make test
```

#### Run individual tests

For debugging a single test execute the following:

```
ctest -R event::per_event --output-on-failure -V
```

Test output and device server logs are collected in `build/tests/results`.

For more details on testing with CTest, see [here](https://cmake.org/Wiki/CMake/Testing_With_CTest).

#### Setting environment up manually

The test runner automatically starts database and all required
device servers for each test. If you want to set up the environment
manually, from `build/` directory run:

```bash
source ./tests/environment/setup_database.sh  # source to get TANGO_HOST
./tests/environment/setup_devices.sh
# attach the debugger or perform some additional configuration
TANGO_TEST_CASE_SKIP_FIXTURE=1 ctest -V -R ds_cache
killall DevTest FwdTest
docker stop tango_cs mysql_db
```

#### Execucting gitlab CI jobs locally

For faster development it is possible to execute gitlab CI jobs locally on the development machine.

* Install gitlab runner, see https://docs.gitlab.com/runner/install
* `gitlab-runner exec docker --env "MAKEFLAGS=-j NUMBER_OF_CPUS" JOBNAME`
