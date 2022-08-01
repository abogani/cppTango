
#include "common.h"

#include <stdlib.h>

auto unset_env(const std::string& var) -> int
{
#ifdef _TG_WINDOWS_
    return _putenv_s(var.c_str(), "");
#else
    return unsetenv(var.c_str());
#endif
}

auto set_env(const std::string& var, const std::string& value, bool force_update) -> int
{
#ifdef _TG_WINDOWS_
    return _putenv_s(var.c_str(), value.c_str());
#else
    return setenv(var.c_str(), value.c_str(), force_update);
#endif
}

