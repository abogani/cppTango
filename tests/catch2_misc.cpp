#include "catch2_common.h"

#include <catch2/matchers/catch_matchers.hpp>

template <class Base>
class MiscDev : public Base
{
  public:
    using Base::Base;

    ~MiscDev() override { }

    void init_device() override
    {
        Base::set_state(Tango::ON);
    }

    void set_state_from_ext(Tango::DevState state)
    {
        Base::set_state(state);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&MiscDev::set_state_from_ext>("IOState"));
    }
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(MiscDev, 1)

SCENARIO("Various device properties")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"misc", "MiscDev", idlver};
        auto device = ctx.get_proxy();

        WHEN("we can query Status and State")
        {
            using namespace TangoTest::Matchers;

            Tango::DeviceData status = device->command_inout("Status");
            std::string ref = "The device is in ON state.";
            REQUIRE_THAT(status, AnyLikeContains(ref));

            Tango::DeviceData state = device->command_inout("State");
            REQUIRE_THAT(state, AnyLikeContains(Tango::ON));
        }

        WHEN("we can query name and description")
        {
            REQUIRE(device->name() == "TestServer/tests/1");
            REQUIRE(device->description() == "A TANGO device");
        }

        WHEN("we set the state and restart")
        {
            using namespace TangoTest::Matchers;

            Tango::DeviceData din;
            din << Tango::OFF;
            REQUIRE_NOTHROW(device->command_inout("IOState", din));

            Tango::DeviceData state = device->command_inout("State");
            REQUIRE_THAT(state, AnyLikeContains(Tango::OFF));

            auto dserver = ctx.get_admin_proxy();

            din << device->name();
            REQUIRE_NOTHROW(dserver->command_inout("DevRestart", din));

            // back at the original state
            Tango::DeviceData state_new = device->command_inout("State");
            REQUIRE_THAT(state_new, AnyLikeContains(Tango::ON));
        }
        WHEN("ping the device")
        {
            REQUIRE_NOTHROW(device->ping());
        }
        WHEN("we check the info call")
        {
            using namespace Catch::Matchers;

            auto info = device->info();

            CAPTURE(info);

            REQUIRE(info.dev_class == "MiscDev_" + std::to_string(idlver));
            REQUIRE(info.dev_type == "Uninitialised");
            REQUIRE(info.doc_url == "Doc URL = http://www.tango-controls.org");
            // ease our lives by just checking that the host name is not empty
            REQUIRE_THAT(info.server_host, !IsEmpty());
            REQUIRE(info.server_id == "TestServer/misc");
            REQUIRE(info.server_version == 6);
        }
    }
}
