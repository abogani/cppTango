#ifndef _INTERNAL_MISC_H
#define _INTERNAL_MISC_H

#include <tango/server/tango_config.h>

#include <vector>
#include <string>

#ifndef _TG_WINDOWS_
  #include <unistd.h>
#endif

namespace Tango::detail
{

#if defined(MAXHOSTNAMELEN)
constexpr std::size_t TANGO_MAX_HOSTNAME_LEN{MAXHOSTNAMELEN + 1};
#else
// Windows needs 255 + 1.
// Let's assume that all other OSes also need 255 + 1.
constexpr std::size_t TANGO_MAX_HOSTNAME_LEN{256};
#endif

/// @brief Return true if the given endpoint is a valid IPv4 address
bool is_ip_address(const std::string &endpoint);

/// @brief Return a list of IPv4 adresses of the given hostname
std::vector<std::string> resolve_hostname_address(const std::string &hostname);

/// Turns the hostname/ip-address `name` and `port` into `tcp://$name:$port`
std::string qualify_host_address(std::string name, const std::string &port);

/// Returns the port from `something:port`
std::string get_port_from_endpoint(const std::string &endpoint);

/// Returns the name and port from `tcp://$name:$port
void split_endpoint(const std::string &endpoint, std::string &name, std::string &port);

/// Returns the ip address/hostname of a CORBA URI
///
/// Returns `myhost` when given giop:tcp:myhost:12345.
///
/// As specified by omniORB both port and hostname can be empty.
std::string parse_hostname_from_CORBA_URI(const std::string &input);

} // namespace Tango::detail

#endif // _INTERNAL_MISC_H
