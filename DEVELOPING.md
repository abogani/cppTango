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
