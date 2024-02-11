// NOLINTBEGIN(*)

#include "common.h"

#include <stdlib.h>

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
double parse_as<double>(const std::string &str)
{
    char *end = nullptr;

    errno = 0;
    double result = strtod(str.c_str(), &end);

    if(str.size() == 0 || errno == ERANGE || end != str.data() + str.size())
    {
        std::stringstream ss;
        ss << "\"" << str << "\" cannot be entirely parsed into double";
        throw std::runtime_error{ss.str()};
    }

    return result;
}
#endif

// NOLINTEND(*)
