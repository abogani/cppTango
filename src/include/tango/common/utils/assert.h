#ifndef TANGO_COMMON_UTILS_ASSERT_H
#define TANGO_COMMON_UTILS_ASSERT_H

#include <tango/server/tango_current_function.h>

/** @defgroup Macros Common macros
 */

namespace Tango::detail
{
///@privatesection

// Called by TANGO_ASSERT on assertion failure.  Not to be called directly.
//
// See TANGO_ASSERT documentation for details.
//
// @param [in] file Source file where assertion failed
// @param [in] line Line number where assertion failed
// @param [in] func Name of the function where assertion failed
// @param [in] msg  Message to display
[[noreturn]] void assertion_failure(const char *file, int line, const char *func, const char *msg);
} // namespace Tango::detail

/**
 * Assert condition \a X holds
 *
 * For debug builds asserts that condition \a X is true.  If not abort the
 * program with std::terminate after printing  the location of
 * the assertion and the asserted expression to stderr and the Tango API_LOGGER,
 * if the later is available.
 *
 * @param [in] X Condition to assert
 * @ingroup Macros
 */

#ifndef NDEBUG
  #define TANGO_ASSERT(X)        \
      (X) ? static_cast<void>(0) \
          : Tango::detail::assertion_failure(__FILE__, __LINE__, TANGO_CURRENT_FUNCTION, "Assertion '" #X "' failed")
#else
  #define TANGO_ASSERT(X) static_cast<void>(X)
#endif

/**
 * Abort on default branch of switch statement
 *
 * For debug builds aborts the program with a message printed to stderr
 * explaining that we have reached an unexpected default branch of a
 * switch statement.
 *
 * @param [in] switchValue Unhandled switch value
 * @ingroup Macros
 */

#ifndef NDEBUG
  #define TANGO_ASSERT_ON_DEFAULT(switchValue)                                                             \
      do                                                                                                   \
      {                                                                                                    \
          using std::to_string;                                                                            \
          std::stringstream msg;                                                                           \
          msg << "Reached unexpected default branch with value '" << to_string(switchValue) << "'";        \
          Tango::detail::assertion_failure(__FILE__, __LINE__, TANGO_CURRENT_FUNCTION, msg.str().c_str()); \
      } while(false)
#else
  #define TANGO_ASSERT_ON_DEFAULT(switchValue) static_cast<void>(switchValue)
#endif

#endif
