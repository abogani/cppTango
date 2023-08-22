This document is targeted at cppTango developers only.

# Generating code coverage report

> *Note: Code coverage reports are currently supported only with GCC or Clang.*

To generate code coverage report one can follow these steps:

1. Enable code coverage support with CMake flag `TANGO_ENABLE_COVERAGE`:
   ```
   cmake -DTANGO_ENABLE_COVERAGE=ON ...
   ```
2. Run any relevant tests that contribute to the code coverage:
   ```
   ctest ...
   ```
3. Generate the report with a tool of choice, e.g. `gcovr` can generate reports
   in many formats. Note that usually one is interested only in the library
   code and may want to exclude any test code from the report.
   Below command can be run from the project's root directory:
   ```
   gcovr --filter '^src/' --filter '^log4tango/(?!tests/)' --html-details --output coverage.html
   ```

# Using clang-tidy for code quality checks

`clang-tidy` can be used to detect potential bugs and code quality issues.
Below are some comments worth noticing:

* to generate the compilation database one could pass the standard
  `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` option when running CMake.
* when using unity build, one must use `header-filter` option to include
  at least the .cpp files, otherwise all warnings will be ignored,
* it is recomended to use `header-filter='.*'` to still see the warnings which
  are not related to any source file,
* default [configuration file](.clang-tidy) contains a list of checks that
  are verified in the CI pipeline.

Example commands to run clang-tidy on all files (excluding the tests):
```
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_UNITY_BUILD=ON -DCMAKE_UNITY_BUILD_BATCH_SIZE=0 ...
run-clang-tidy.py -header-filter='.*' 'src/(?!server/idl)' 'log4tango/src'
```

# Code style checks

Some of our CI jobs serve as early merge gate to detect easy to fix code style
issues. All other CI jobs, which are more expensive to run, depend on those.

## Pre-commit

A [framework](https://pre-commit.com) to perform all kinds of checks.
Can be installed via `pip install pre-commit`.

Running the checks locally:

```
pre-commit run --all-files --show-diff-on-failure
```

pre-commit can also be integrated into git, see
[here](https://pre-commit.com/#3-install-the-git-hook-scripts) for instructions.

## Ignoring commits for git blame

The commits in the file `.git-blame-ignore-revs` are meant to be ignored for invocations of
git blame as they contain tree-wide scripted changes only.

The following command sets that up:

```
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

## Code formatting

- Our coding style is defined via a .clang-format file in the top level directory.
  Please configure your editor to use that. In case your editor does not
  support auto-formatting, you can enable the `clang-format` via
  `cmake -S . -B build -DTANGO_CLANG_FORMAT=ON` and call it like `cmake --build build --target clang-format`
- Please refrain from adding "fix formatting" commits as these are in general
  not necessary and will also not be accepted.
- The current reference version of clang-format is 16, packages are available
  for [debian](https://apt.llvm.org),
  [fedora](https://copr.fedorainfracloud.org/coprs/g/fedora-llvm-team/llvm-snapshots/),
  [MacOSX](https://formulae.brew.sh/formula/clang-format#default),
  [Windows](https://github.com/llvm/llvm-project/releases) and possibly more.

For special cases code formatting can be turned off inline, see
[here](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#disabling-formatting-on-a-piece-of-code)
for details.
