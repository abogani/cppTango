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

  void test_nothing()
  {
    TS_ASSERT(true);
  }
};

#endif // CommonMiscTestSuite_h
