#ifndef TANGO_INTERNAL_STL_CORBA_HELPERS_H
#define TANGO_INTERNAL_STL_CORBA_HELPERS_H

#include <tango/tango.h>

#include <tango/internal/type_traits.h>

#include <type_traits>

// This file implements helper functions which allows CORBA types to be used like standard STL containers
// Useful for AnyMatch/AllMatch/IsEmpty/SizeIs catch2 matchers or range-based STL code

namespace Tango
{
/// Overloads for CORBA Sequences
///@{

template <typename T>
inline size_t size(const _CORBA_Sequence<T> &seq)
{
    return seq.length();
}

template <typename T>
inline bool empty(const _CORBA_Sequence<T> &seq)
{
    return size(seq) == 0u;
}

template <typename T>
inline auto begin(_CORBA_Sequence<T> &seq)
{
    return seq.NP_data();
}

template <typename T>
inline auto begin(const _CORBA_Sequence<T> &seq)
{
    return seq.NP_data();
}

template <typename T>
inline auto cbegin(const _CORBA_Sequence<T> &seq)
{
    return begin(seq);
}

template <typename T>
inline auto end(_CORBA_Sequence<T> &seq)
{
    return seq.NP_data() + seq.length();
}

template <typename T>
inline auto end(const _CORBA_Sequence<T> &seq)
{
    return seq.NP_data() + seq.length();
}

template <typename T>
inline auto cend(const _CORBA_Sequence<T> &seq)
{
    return end(seq);
}

///@}

/// Overloads for CORBA var classes holding CORBA Sequences
///@{

template <typename T, typename std::enable_if_t<detail::is_corba_var_from_seq_v<T>, bool> = true>
inline size_t size(const T &var)
{
    auto cont = var.operator->();
    if(cont == nullptr)
    {
        return 0u;
    }

    return size(*cont);
}

template <typename T, typename std::enable_if_t<detail::is_corba_var_from_seq_v<T>, bool> = true>
inline bool empty(const T &var)
{
    auto cont = var.operator->();
    if(cont == nullptr)
    {
        return true;
    }

    return empty(*cont);
}

template <typename T, typename std::enable_if_t<detail::is_corba_var_from_seq_v<T>, bool> = true>
inline decltype(auto) begin(const T &var)
{
    auto cont = var.operator->();
    if(cont == nullptr)
    {
        using ConstElementPtr = std::add_const_t<std::add_pointer_t<detail::corba_ut_from_var_from_seq_t<T>>>;
        return static_cast<ConstElementPtr>(nullptr);
    }

    return cbegin(*cont);
}

template <typename T, typename std::enable_if_t<detail::is_corba_var_from_seq_v<T>, bool> = true>
inline decltype(auto) begin(T &var)
{
    auto cont = var.operator->();
    if(cont == nullptr)
    {
        using ElementPtr = std::add_pointer_t<detail::corba_ut_from_var_from_seq_t<T>>;
        return static_cast<ElementPtr>(nullptr);
    }

    return begin(*cont);
}

template <typename T, typename std::enable_if_t<detail::is_corba_var_from_seq_v<T>, bool> = true>
inline decltype(auto) cbegin(const T &var)
{
    return begin(var);
}

template <typename T, typename std::enable_if_t<detail::is_corba_var_from_seq_v<T>, bool> = true>
inline decltype(auto) end(T &var)
{
    auto cont = var.operator->();
    if(cont == nullptr)
    {
        using ElementPtr = std::add_pointer_t<detail::corba_ut_from_var_from_seq_t<T>>;
        return static_cast<ElementPtr>(nullptr);
    }

    return end(*cont);
}

template <typename T, typename std::enable_if_t<detail::is_corba_var_from_seq_v<T>, bool> = true>
inline decltype(auto) end(const T &var)
{
    auto cont = var.operator->();
    if(cont == nullptr)
    {
        using ConstElementPtr = std::add_const_t<std::add_pointer_t<detail::corba_ut_from_var_from_seq_t<T>>>;
        return static_cast<ConstElementPtr>(nullptr);
    }

    return end(*cont);
}

template <typename T, typename std::enable_if_t<detail::is_corba_var_from_seq_v<T>, bool> = true>
inline decltype(auto) cend(const T &var)
{
    return end(var);
}

///@}

} // namespace Tango

#endif
