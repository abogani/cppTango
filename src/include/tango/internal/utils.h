#ifndef _INTERNAL_UTILS_H
#define _INTERNAL_UTILS_H

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace Tango::detail
{

constexpr int INVALID_IDL_VERSION = 0;

/// @brief Check wether the given IDL version is at least the desired IDL version
///
/// Helper function for DeviceProxy/MultiAttribute which ignores unconnected
/// devices with version 0 (which is not a valid IDL version).
bool IDLVersionIsTooOld(int version, int desiredVersion);

/// @brief Helper template to check if the type T is one of the contained types in the variant U
///
/// @code
/// static_assert(Tango::detail::is_one_of<bool, myVarientType, "Unsupported type");
/// @endcode
template <class T, class U>
struct is_one_of;

template <class T, class... Ts>
struct is_one_of<T, std::variant<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)>
{
};

template <typename T>
void stringify_vector(std::ostream &os, const std::vector<T> &vec, const std::string_view sep)
{
    const auto length = vec.size();
    if(length == 0)
    {
        return;
    }

    typename std::vector<T>::size_type i;

    TANGO_ASSERT(length > 0);
    for(i = 0; i < (length - 1); i++)
    {
        os << vec[i] << sep;
    }

    os << vec[i];
}

/// @brief Convert the given string to lower case
std::string to_lower(std::string str);

/// @brief Convert the given string to UPPER case
std::string to_upper(std::string str);

/// @brief Parse the given lower case string as boolean
///
/// Returns an optional without value in case of error.
std::optional<bool> to_boolean(std::string_view str);

/// @brief Lookup the environment variable `env_var` and return its contents as boolean
///
/// Return `default_value` in case it is not present, throws for unkonwn content
bool get_boolean_env_var(const char *env_var, bool default_value);

} // namespace Tango::detail

#endif // _INTERNAL_UTILS_H
