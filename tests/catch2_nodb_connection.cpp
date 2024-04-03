#include <tango/tango.h>
#include "utils/utils.h"

SCENARIO("Connection to invalid nodb device name")
{
    REQUIRE_NOTHROW(Tango::DeviceProxy("tango://localhost:0/invalid/test/dev#dbase=no"));
}
