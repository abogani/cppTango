// NOLINTBEGIN(*)

#include "old_common.h"

#include <stdlib.h>
#include <iostream>

auto unset_env(const std::string &var) -> int
{
#ifdef _TG_WINDOWS_
    return _putenv_s(var.c_str(), "");
#else
    return unsetenv(var.c_str());
#endif
}

auto set_env(const std::string &var, const std::string &value, bool force_update) -> int
{
#ifdef _TG_WINDOWS_
    return _putenv_s(var.c_str(), value.c_str());
#else
    return setenv(var.c_str(), value.c_str(), force_update);
#endif
}

#ifndef TANGO_HAS_FROM_CHARS_DOUBLE
template <>
double parse_as<double>(std::string_view str)
{
    char *end = nullptr;

    // We have to copy the string_view as it might not be nul-terminated.
    std::string copy{str};

    errno = 0;
    double result = strtod(copy.c_str(), &end);

    if(str.size() == 0 || errno == ERANGE || end != copy.data() + copy.size())
    {
        std::stringstream ss;
        ss << "\"" << str << "\" cannot be entirely parsed into double";
        throw std::runtime_error{ss.str()};
    }

    return result;
}
#endif

std::string load_file(const std::string &file)
{
    std::ifstream ifs(file, std::ios::binary);

    return {std::istreambuf_iterator<char>{ifs}, {}};
}

// NOLINTEND(*)
