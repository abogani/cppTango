#ifndef _INTERNAL_UTILS_H
#define _INTERNAL_UTILS_H

#include <type_traits>
#include <variant>

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

} // namespace Tango::detail

#endif // _INTERNAL_UTILS_H
