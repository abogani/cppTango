#ifndef _INTERNAL_MISC_H
#define _INTERNAL_MISC_H

#include <vector>
#include <string>

namespace Tango
{
namespace detail
{

/// @brief Return true if the given endpoint is a valid IPv4 address
bool is_ip_address(const std::string &endpoint);

/// @brief Return a list of IPv4 adresses of the given hostname
std::vector<std::string> resolve_hostname_address(const std::string &hostname);

/// Turns the hostname/ip-address `name` and `port` into `tcp://$name:$port`
std::string qualify_host_address(std::string name, const std::string &port);

/// Returns the port from `something:port`
std::string get_port_from_endpoint(const std::string& endpoint);

} // namespace detail
} // namespace Tango

#endif // _INTERNAL_MISC_H
