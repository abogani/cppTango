#ifndef TANGO_INTERNAL_TYPE_TRAITS_H
#define TANGO_INTERNAL_TYPE_TRAITS_H

#include <type_traits>

namespace Tango::detail
{
template <typename AlwaysVoid, template <typename...> class Op, typename... Args>
struct is_detected : std::false_type
{
};

template <template <typename...> class Op, typename... Args>
struct is_detected<std::void_t<Op<Args...>>, Op, Args...> : std::true_type
{
};

/**
 * @brief true if Op<Args...> is a valid type
 *
 * Inspired by std::experimental::is_detected.
 *
 * For example, to check that <expr> is valid for a type T:
 *
 *  template <typename T>
 *  using expr_t = decltype(<expr>);
 *
 *  template <typename T>
 *  constexpr bool supports_expr = is_detected_v<expr_t, T>;
 *
 * @tparam Op template to construct a type
 * @tparam Args arguments to pass to the template
 */
template <template <typename...> class Op, typename... Args>
constexpr bool is_detected_v = is_detected<void, Op, Args...>::value;
} // namespace Tango::detail

#endif
