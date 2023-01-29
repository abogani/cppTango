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
};

#endif // CommonMiscTestSuite_h
