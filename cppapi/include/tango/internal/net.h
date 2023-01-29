#ifndef _INTERNAL_MISC_H
#define _INTERNAL_MISC_H

namespace Tango
{
namespace detail
{

/// @brief Return true if the given endpoint is a valid IPv4 address
bool is_ip_address(const std::string &endpoint);

} // namespace detail
} // namespace Tango

#endif // _INTERNAL_MISC_H
