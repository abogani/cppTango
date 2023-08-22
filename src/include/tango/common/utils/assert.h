#ifndef TANGO_COMMON_UTILS_ASSERT_H
#define TANGO_COMMON_UTILS_ASSERT_H

#include <tango/server/tango_current_function.h>

/** @defgroup Macros Common macros
 */

namespace Tango
{
namespace detail
{
///@privatesection

// Called by TANGO_ASSERT on assertion failure.  Not to be called directly.
//
// See TANGO_ASSERT documentation for details.
//
// @param [in] file Source file where assertion failed
// @param [in] line Line number where assertion failed
// @param [in] func Name of the function where assertion failed
// @param [in] expr Asserted expression
[[noreturn]] void assertion_failure(const char *file, int line, const char *func, const char *expr);
} // namespace detail
} // namespace Tango

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
  #define TANGO_ASSERT(X) \
      (X) ? static_cast<void>(0) : Tango::detail::assertion_failure(__FILE__, __LINE__, TANGO_CURRENT_FUNCTION, #X)
#else
  #define TANGO_ASSERT(X) static_cast<void>(X)
#endif

#endif
