#include "utils/utils.h"

SCENARIO("We can launch multiple servers")
{
    GIVEN("A context description with a pair of servers")
    {
        TangoTest::ContextDescriptor desc;

        desc.servers.push_back(TangoTest::ServerDescriptor{"1", "Empty"});
        desc.servers.push_back(TangoTest::ServerDescriptor{"2", "Empty"});

        WHEN("We construct a context with that description")
        {
            TangoTest::Context ctx{desc};

            THEN("We can ping both devices")
            {
                auto device1 = ctx.get_proxy("1");
                REQUIRE_NOTHROW(device1->ping());

                auto device2 = ctx.get_proxy("2");
                REQUIRE_NOTHROW(device2->ping());

                AND_THEN("The devices are different")
                {
                    REQUIRE(device1->name() != device2->name());
                }
            }
        }
    }
}
