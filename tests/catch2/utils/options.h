#ifndef TANGO_TESTS_CATCH2_UTILS_OPTIONS_H
#define TANGO_TESTS_CATCH2_UTILS_OPTIONS_H

#include <optional>
#include <string>

namespace TangoTest
{

struct Options
{
    bool log_file_per_test_case = false;
    std::optional<int> only_idl_version;
};

extern Options g_options;

} // namespace TangoTest

#endif
