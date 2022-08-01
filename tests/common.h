#ifndef Common_H
#define Common_H

#include <iostream>
#include <tango/tango.h>

#ifndef TS_ASSERT
#include <assert.h>
#endif

#ifdef WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

using namespace std;
using namespace Tango;

#include "logging.h"

/**
 * Wrapper for unsetenv working on windows systems as well.
 * Unset environment variable var
 */
auto unset_env(const std::string& var) -> int;

/**
 * Wrapper for setenv working on windows systems as well.
 * Set environment variable var to value value.
 * Force update if force_update flag is set to true (not used on windows)
 */
auto set_env(const std::string& var, const std::string& value, bool force_update) -> int;

#endif // Common_H
