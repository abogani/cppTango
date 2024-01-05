#ifndef _INTERNAL_UTILS_H
#define _INTERNAL_UTILS_H

namespace Tango::detail
{

constexpr int INVALID_IDL_VERSION = 0;

/// @brief Check wether the given IDL version is at least the desired IDL version
///
/// Helper function for DeviceProxy/MultiAttribute which ignores unconnected
/// devices with version 0 (which is not a valid IDL version).
bool IDLVersionIsTooOld(int version, int desiredVersion);

} // namespace Tango::detail

#endif // _INTERNAL_UTILS_H
