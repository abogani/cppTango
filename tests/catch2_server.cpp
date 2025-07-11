#include "catch2_common.h"

SCENARIO("The device server can be killed")
{
    GIVEN("a device proxy to a dserver device")
    {
        TangoTest::Context ctx{"dserver_kill", "Empty"};
        auto admin = ctx.get_admin_proxy();

        WHEN("we call the Kill command")
        {
            REQUIRE_NOTHROW(admin->command_inout("Kill"));

            THEN("the device server stops soon")
            {
                using namespace TangoTest::Matchers;

                REQUIRE_THAT(ctx.wait_for_exit(), IsSuccess());
            }
        }
    }
}
