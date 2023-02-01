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

std::vector<std::string> resolve_hostname_address(const std::string &hostname)
{
  if(hostname.empty())
  {
    TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, "Can not work with an empty hostname");
  }

  struct addrinfo hints{};

  hints.ai_flags     = AI_ADDRCONFIG;
  hints.ai_family    = AF_INET;
  hints.ai_socktype  = SOCK_STREAM;

  struct addrinfo *info = nullptr;

  int result = getaddrinfo(hostname.c_str(), NULL, &hints, &info);

  if (result != 0)
  {
    std::stringstream o;
    o << "Can't convert " << hostname << " to IP address";

    TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, o.str());
  }

  std::vector<std::string> results;

  for(struct addrinfo *ptr = info; ptr != nullptr; ptr = ptr->ai_next)
  {
    struct sockaddr_in *s_in = (sockaddr_in *) ptr->ai_addr;
    results.push_back(inet_ntoa(s_in->sin_addr));
  }

  freeaddrinfo(info);
  info = nullptr;

  return results;
}

std::string get_port_from_endpoint(const std::string& endpoint)
{
  auto invalid_args_assert = [&endpoint] (bool cond)
  {
    if(cond)
      return;

    TangoSys_OMemStream o;
    o << "Could not extract the port from " << "\"" << endpoint << "\"" << std::ends;

    TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, o.str());
  };

  std::string::size_type pos = endpoint.rfind(':');
  invalid_args_assert(pos != std::string::npos);

  auto port = endpoint.substr(pos + 1);
  invalid_args_assert(!port.empty());

  return port;
}

void split_endpoint(const std::string &endpoint, std::string &name, std::string &port)
{
  name.clear();
  port.clear();

  auto invalid_args_assert = [&endpoint] (bool cond)
  {
    if(cond)
      return;

    TangoSys_OMemStream o;
    o << "Could not extract name and port from " << "\"" << endpoint << "\"" << std::ends;

    TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, o.str());
  };

  std::string::size_type pos = endpoint.rfind(':');
  invalid_args_assert(pos != std::string::npos);

  // 6 is the length of tcp://
  std::string name_temp = endpoint.substr(6, pos - 6);
  invalid_args_assert(!name_temp.empty());

  std::string port_temp = endpoint.substr(pos + 1);
  invalid_args_assert(!port_temp.empty());

  // all good now
  name = name_temp;
  port = port_temp;
}

std::string qualify_host_address(std::string name, const std::string &port)
{
  auto invalid_args_assert = [&name, &port] (bool cond)
  {
    if(cond)
      return;

    TangoSys_OMemStream o;
    o << "Neither name " << "\"" << name << "\"" << " nor port " << "\" " << port << "\" ";
    o << "can be empty" << std::ends;

    TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, o.str());
  };

  invalid_args_assert(!name.empty());
  invalid_args_assert(!port.empty());

  name.insert(0, "tcp://");
  name.append(":");
  name.append(port);

  return name;
}

} // namespace detail
} // namespace Tango
