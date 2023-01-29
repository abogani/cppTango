#include <tango/tango.h>
#include <tango/internal/net.h>

#ifdef _TG_WINDOWS_
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif /* _TG_WINDOWS_ */

namespace Tango
{
namespace detail
{

bool is_ip_address(const std::string &endpoint)
{
  if(endpoint.empty())
  {
    TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, "Can not work with an empty endpoint");
  }

#ifndef _TG_WINDOWS_
  unsigned char buf[sizeof(struct in_addr)];
  return inet_pton(AF_INET, endpoint.c_str(), buf) == 1;
#else
  struct sockaddr_storage ss;
  int size = sizeof(ss);
  char src_copy[INET6_ADDRSTRLEN+1];

  ZeroMemory(&ss, sizeof(ss));
  strncpy (src_copy, endpoint.c_str(), INET6_ADDRSTRLEN + 1);
  src_copy[INET6_ADDRSTRLEN] = 0;

  return WSAStringToAddress(src_copy, AF_INET, NULL, (struct sockaddr *)&ss ,&size) == 0;
#endif
}

} // namespace detail
} // namespace Tango
