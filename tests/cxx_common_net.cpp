#ifndef CommonMiscTestSuite_h
#define CommonMiscTestSuite_h

#include "cxx_common.h"

#include <tango/internal/net.h>
using namespace Tango::detail;

#undef SUITE_NAME
#define SUITE_NAME CommonMiscTestSuite

class SUITE_NAME: public CxxTest::TestSuite
{
protected:

public:
  SUITE_NAME()
  {
//
// Arguments check -------------------------------------------------
//

    CxxTest::TangoPrinter::validate_args();

//
// Initialization --------------------------------------------------
//
  }

  virtual ~SUITE_NAME() = default;

  static SUITE_NAME *createSuite()
  {
    return new SUITE_NAME();
  }

  static void destroySuite(SUITE_NAME *suite)
  {
    delete suite;
  }

//
// Tests -------------------------------------------------------
//

  void test_is_ip_address()
  {
    // IPv4
    TS_ASSERT(is_ip_address("127.0.0.1"));
    TS_ASSERT(is_ip_address("1.1.1.1"));

    // IPv6 is not supported
    TS_ASSERT(!is_ip_address("::1"));

    // random strings
    TS_ASSERT(!is_ip_address("example_dot_org"));

    // hostname
    TS_ASSERT(!is_ip_address("example.org"));

    // we need a non empty string
    TS_ASSERT_THROWS_ASSERT(is_ip_address(""), Tango::DevFailed &e, TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), API_InvalidArgs));
  }

  void test_resolve_hostname_address()
  {
    std::vector<std::string> results;
    TS_ASSERT_THROWS_NOTHING(results = resolve_hostname_address("localhost"));
    TS_ASSERT(results.size() >= 1);
    TS_ASSERT(std::find(std::cbegin(results), std::cend(results), "127.0.0.1") != std::end(results));

    // IPv4 only
    TS_ASSERT_THROWS_NOTHING(results = resolve_hostname_address("ip6-loopback"));
    TS_ASSERT(results.size() >= 1);
    TS_ASSERT(std::find(std::cbegin(results), std::cend(results), "127.0.0.1") != std::end(results));
    TS_ASSERT(std::find(std::cbegin(results), std::cend(results), "::1") == std::end(results));

    // invalid hostname
    TS_ASSERT_THROWS_ASSERT(auto result = resolve_hostname_address("I_DONT_EXIST..com"), Tango::DevFailed &e, TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), API_InvalidArgs));

    // unresolvable hostname
    TS_ASSERT_THROWS_ASSERT(auto result = resolve_hostname_address("I_DONT_EXIST.AT.NON_EXISTING_SUBDOMAIN.byte-physics.de"), Tango::DevFailed &e, TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), API_InvalidArgs));

    // we need a non empty string
    TS_ASSERT_THROWS_ASSERT(auto result = resolve_hostname_address(""), Tango::DevFailed &e, TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), API_InvalidArgs));
  }

  void test_get_port_from_endpoint()
  {
    // empty
    TS_ASSERT_THROWS_ASSERT(auto port = get_port_from_endpoint(""), Tango::DevFailed &e, TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), API_InvalidArgs));

    // invalid format without :
    TS_ASSERT_THROWS_ASSERT(auto port = get_port_from_endpoint("a_b"), Tango::DevFailed &e, TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), API_InvalidArgs));

    // invalid format as : is the last char
    TS_ASSERT_THROWS_ASSERT(auto port = get_port_from_endpoint("b:"), Tango::DevFailed &e, TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), API_InvalidArgs));

    TS_ASSERT_EQUALS(get_port_from_endpoint("a:b"), "b");
    TS_ASSERT_EQUALS(get_port_from_endpoint("tcp://a:b"), "b");
  }

  void test_qualify_host_address()
  {
    // empty name
    TS_ASSERT_THROWS_ASSERT(qualify_host_address("", "b"), Tango::DevFailed &e, TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), API_InvalidArgs));

    // empty port
    TS_ASSERT_THROWS_ASSERT(qualify_host_address("a", ""), Tango::DevFailed &e, TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), API_InvalidArgs));

    TS_ASSERT_EQUALS(qualify_host_address("a", "b"), "tcp://a:b");
  }
};

#endif // CommonMiscTestSuite_h
