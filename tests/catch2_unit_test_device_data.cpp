#include "catch2_common.h"

#include <catch2/matchers/catch_matchers.hpp>

SCENARIO("check is_empty")
{
    using namespace Catch::Matchers;
    using namespace TangoTest::Matchers;

    GIVEN("an empty DeviceData")
    {
        Tango::DeviceData dd;

        WHEN("we throw")
        {
            REQUIRE_THROWS_MATCHES(
                dd.is_empty(), Tango::DevFailed, FirstErrorMatches(Reason(Tango::API_EmptyDeviceData)));
        }
        WHEN("we don't throw if exceptions are disabled")
        {
            // disable exceptions
            std::bitset<Tango::DeviceData::numFlags> bs;
            dd.exceptions(bs);
            REQUIRE(dd.is_empty());
        }
    }
    GIVEN("a filled DeviceData")
    {
        Tango::DeviceData dd;
        dd << true;

        WHEN("we return truth")
        {
            REQUIRE(!dd.is_empty());
        }
    }
}

SCENARIO("check get_type")
{
    using namespace Catch::Matchers;
    using namespace TangoTest::Matchers;

    GIVEN("an empty DeviceData")
    {
        Tango::DeviceData dd;
        std::bitset<Tango::DeviceData::numFlags> bs;
        dd.exceptions(bs);

        WHEN("we return -1")
        {
            REQUIRE(dd.get_type() == -1);
        }
    }
    GIVEN("a filled DeviceData")
    {
        Tango::DeviceData dd;
        dd << true;

        WHEN("we return truth")
        {
            REQUIRE(dd.get_type() == Tango::DEV_BOOLEAN);
        }
    }
    GIVEN("a filled DeviceData with an incompatible type")
    {
        Tango::DeviceData dd;
        CORBA::Any_var any = new CORBA::Any;
        dd.any <<= any;

        WHEN("we throw")
        {
            REQUIRE_THROWS_MATCHES(
                dd.get_type(), Tango::DevFailed, FirstErrorMatches(Reason(Tango::API_InvalidCorbaAny)));
        }
    }
}
