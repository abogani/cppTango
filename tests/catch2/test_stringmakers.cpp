#include <tango/tango.h>

#include "utils/utils.h"

SCENARIO("catch2 stringmakers specialications")
{
    GIVEN("a TimeVal")
    {
        Tango::TimeVal val;
        val.tv_sec = 1;
        val.tv_usec = 2;
        val.tv_nsec = 3;

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::TimeVal>::convert(val);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    GIVEN("a EventData")
    {
        Tango::EventData evData;
        evData.device = nullptr;
        evData.attr_value = nullptr;
        evData.err = false;

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::EventData>::convert(evData);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    GIVEN("a DeviceInfo")
    {
        Tango::DeviceInfo info;

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::DeviceInfo>::convert(info);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }
}
