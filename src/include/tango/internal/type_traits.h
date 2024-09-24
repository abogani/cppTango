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

/// Determine if T has the `_retn` member function aka T is a CORBA var class
template <typename T>
using corba_var_t = decltype(&T::_retn);

/// Type traits for inspecting CORBA var classes and CORBA sequences
///@{

/// Determine the underlying type of a CORBA var class
template <typename T>
using corba_ut_from_var_t = std::remove_pointer_t<std::invoke_result_t<corba_var_t<T>, T>>;

/// Determine if T is a CORBA var class
template <typename T>
constexpr bool is_corba_var_v = Tango::detail::is_detected_v<corba_var_t, T>;

/// Determine if T has the `NP_data` member function aka T is a CORBA sequence class
template <typename T>
using corba_seq_t = decltype(&T::NP_data);

/// Determine the underlying type of a CORBA sequence class
template <typename T>
using corba_ut_from_seq_t = std::remove_pointer_t<std::invoke_result_t<corba_seq_t<T>, T>>;

/// Determine if T is a CORBA sequence class
template <typename T>
constexpr bool is_corba_seq_v = Tango::detail::is_detected_v<corba_seq_t, T>;

/// Determine the underlying type of a CORBA sequence class given by a CORBA var class
template <typename T>
using corba_ut_from_var_from_seq_t = corba_ut_from_seq_t<corba_ut_from_var_t<T>>;

template <typename T>
constexpr bool is_corba_var_from_seq_v = Tango::detail::is_detected_v<corba_ut_from_var_from_seq_t, T>;
///@}

} // namespace Tango::detail

#endif
