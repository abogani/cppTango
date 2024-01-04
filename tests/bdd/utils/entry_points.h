#ifndef TANGO_TESTS_BDD_UTILS_ENTRY_POINTS_H
#define TANGO_TESTS_BDD_UTILS_ENTRY_POINTS_H

// To ensure that the executables must link to the library, we define the main
// functions in the library to be called from the executables main.
namespace TangoTest
{
int test_main(int argc, const char *argv[]);
int server_main(int argc, const char *argv[]);
} // namespace TangoTest

#endif
