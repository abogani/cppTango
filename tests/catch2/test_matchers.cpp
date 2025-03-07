#include <tango/tango.h>

#include "utils/utils.h"

#include <chrono>

SCENARIO("catch2 TimeVal matchers")
{
    GIVEN("a TimeVal")
    {
        Tango::TimeVal val;
        val.tv_sec = 1;
        val.tv_usec = 2;
        val.tv_nsec = 4;

        WHEN("we can convert it to a string")
        {
            using namespace TangoTest::Matchers;
            using namespace std::literals::chrono_literals;

            Tango::TimeVal ref;
            ref.tv_sec = 1;
            ref.tv_usec = 3;
            ref.tv_nsec = 4;

            REQUIRE_THAT(val, WithinTimeAbsMatcher(val, 0ns));
            REQUIRE_THAT(val, WithinTimeAbsMatcher(val, 1h));
            REQUIRE_THAT(val, !WithinTimeAbsMatcher(ref, 0us));
            REQUIRE_THAT(val, WithinTimeAbsMatcher(ref, 1us));
        }
    }
}
