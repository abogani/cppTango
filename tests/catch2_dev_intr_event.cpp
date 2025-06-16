#include "catch2_common.h"

namespace
{
const std::string cmd_basename = "Added_cmd";
using CallbackMockType = TangoTest::CallbackMock<Tango::DevIntrChangeEventData>;

class DynCommand : public Tango::Command
{
  public:
    explicit DynCommand(const std::string &name) :
        Tango::Command(name, Tango::DEV_VOID, Tango::DEV_VOID, "", "")
    {
    }

    CORBA::Any *execute(Tango::DeviceImpl *, const CORBA::Any &) override
    {
        auto any = new CORBA::Any;
        return any;
    }
};

} // anonymous namespace

template <class Base>
class DevInterEventDS : public Base
{
  public:
    using Base::Base;

    void init_device() override { }

    void add_command(Tango::DevLong cmd_arg)
    {
        bool device_level = false;
        if(cmd_arg >= 1)
        {
            device_level = true;
        }

        int loop = 1;
        if(cmd_arg == 2)
        {
            loop = 3;
        }

        for(int ctr = 0; ctr < loop; ctr++)
        {
            std::ostringstream ss;
            ss << cmd_basename;
            if(ctr != 0)
            {
                ss << "_" << ctr;
            }

            auto *at = new DynCommand(ss.str());
            Base::add_command(at, device_level);
        }
    }

    void rm_command()
    {
        Base::remove_command(cmd_basename, true);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&DevInterEventDS::add_command>("IOAddCommand"));
        cmds.push_back(new TangoTest::AutoCommand<&DevInterEventDS::rm_command>("IORemoveCommand"));
    }
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(DevInterEventDS, 5)

SCENARIO("DevIntrChangeEventData with dynamic commands")
{
    int idlver = GENERATE(TangoTest::idlversion(Tango::MIN_IDL_DEV_INTR));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"die", "DevInterEventDS", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        std::vector<std::string> static_commands = {"Init", "Status", "State", "IOAddCommand", "IORemoveCommand"};
        std::vector<std::string> static_attributes = {"Status", "State"};

        AND_GIVEN("a subscription to the interface change event")
        {
            CallbackMockType cb;
            TangoTest::Subscription sub{device, Tango::INTERFACE_CHANGE_EVENT, &cb};

            THEN("we already got an interface change event")
            {
                auto event = cb.pop_next_event();
                REQUIRE(event != std::nullopt);

                using namespace TangoTest::Matchers;
                using namespace Catch::Matchers;

                REQUIRE_THAT(event, EventDeviceStarted(true));
                REQUIRE_THAT(event, EventType(Tango::INTERFACE_CHANGE_EVENT));
                REQUIRE_THAT(event, EventCommandNamesMatches(UnorderedRangeEquals(static_commands)));
                REQUIRE_THAT(event, EventAttributeNamesMatches(UnorderedRangeEquals(static_attributes)));

                AND_THEN("we execute a DevRestart command (without adding/removing commands first)")
                {
                    auto dserver = ctx.get_admin_proxy();

                    Tango::DeviceData din;
                    din << device->name();
                    REQUIRE_NOTHROW(dserver->command_inout("DevRestart", din));
                    std::this_thread::sleep_for(std::chrono::seconds(1));

                    THEN("we get no event")
                    {
                        auto event = cb.pop_next_event();
                        REQUIRE(event == std::nullopt);
                    }
                }

                AND_THEN("we execute a RestartServer command (without adding/removing commands first)")
                {
                    auto dserver = ctx.get_admin_proxy();

                    REQUIRE_NOTHROW(dserver->command_inout("RestartServer"));
                    std::this_thread::sleep_for(std::chrono::seconds(5));

                    THEN("we get no event")
                    {
                        auto event = cb.pop_next_event();
                        REQUIRE(event == std::nullopt);
                    }
                }

                WHEN("we add another command")
                {
                    Tango::DevLong in = 0;
                    Tango::DeviceData d_in;
                    d_in << in;
                    REQUIRE_NOTHROW(device->command_inout("IOAddCommand", d_in));

                    THEN("we got an event with the new command")
                    {
                        auto event = cb.pop_next_event(std::chrono::seconds{1});
                        REQUIRE(event != std::nullopt);

                        using namespace TangoTest::Matchers;
                        using namespace Catch::Matchers;

                        REQUIRE_THAT(event, EventDeviceStarted(false));
                        REQUIRE_THAT(event, EventType(Tango::INTERFACE_CHANGE_EVENT));
                        REQUIRE_THAT(event, EventCommandNamesMatches(SizeIs(static_commands.size() + 1)));
                        REQUIRE_THAT(event, EventCommandNamesMatches(AnyMatch(Equals(cmd_basename))));
                        REQUIRE_THAT(event, EventAttributeNamesMatches(UnorderedRangeEquals(static_attributes)));

                        AND_WHEN("we remove the command again")
                        {
                            REQUIRE_NOTHROW(device->command_inout("IORemoveCommand"));
                            std::this_thread::sleep_for(std::chrono::seconds(3));

                            THEN("we got an event with the command removed again")
                            {
                                auto event = cb.pop_next_event(std::chrono::seconds{1});
                                REQUIRE(event != std::nullopt);

                                using namespace TangoTest::Matchers;
                                using namespace Catch::Matchers;

                                REQUIRE_THAT(event, EventDeviceStarted(false));
                                REQUIRE_THAT(event, EventType(Tango::INTERFACE_CHANGE_EVENT));
                                REQUIRE_THAT(event, EventCommandNamesMatches(UnorderedRangeEquals(static_commands)));
                                REQUIRE_THAT(event,
                                             EventAttributeNamesMatches(UnorderedRangeEquals(static_attributes)));
                            }
                        }
                    }
                }

                WHEN("we add multiple commands in a loop")
                {
                    Tango::DevLong in = 2;
                    Tango::DeviceData d_in;
                    d_in << in;
                    REQUIRE_NOTHROW(device->command_inout("IOAddCommand", d_in));

                    THEN("we get only one event")
                    {
                        auto event = cb.pop_next_event();
                        REQUIRE(event != std::nullopt);

                        auto null_event = cb.pop_next_event();
                        REQUIRE(null_event == std::nullopt);

                        using namespace TangoTest::Matchers;
                        using namespace Catch::Matchers;

                        REQUIRE_THAT(event, EventDeviceStarted(false));
                        REQUIRE_THAT(event, EventType(Tango::INTERFACE_CHANGE_EVENT));
                        REQUIRE_THAT(event, EventCommandNamesMatches(SizeIs(static_commands.size() + 3)));
                        REQUIRE_THAT(event, EventCommandNamesMatches(AnyMatch(Equals(cmd_basename))));
                        std::string cmd_name = cmd_basename + "_1";
                        REQUIRE_THAT(event, EventCommandNamesMatches(AnyMatch(Equals(cmd_name))));
                        cmd_name = cmd_basename + "_2";
                        REQUIRE_THAT(event, EventCommandNamesMatches(AnyMatch(Equals(cmd_name))));
                        REQUIRE_THAT(event, EventAttributeNamesMatches(UnorderedRangeEquals(static_attributes)));

                        AND_THEN("we execute an Init command")
                        {
                            REQUIRE_NOTHROW(device->command_inout("Init"));

                            THEN("we get no event")
                            {
                                auto null_event = cb.pop_next_event();
                                REQUIRE(null_event == std::nullopt);
                            }
                        }

                        AND_THEN("we execute a DevRestart command")
                        {
                            auto dserver = ctx.get_admin_proxy();

                            Tango::DeviceData din;
                            din << device->name();
                            REQUIRE_NOTHROW(dserver->command_inout("DevRestart", din));
                            std::this_thread::sleep_for(std::chrono::seconds(1));

                            THEN("we get an event")
                            {
                                auto event = cb.pop_next_event();
                                REQUIRE(event != std::nullopt);

                                REQUIRE_THAT(event, EventDeviceStarted(false));
                                REQUIRE_THAT(event, EventType(Tango::INTERFACE_CHANGE_EVENT));
                                REQUIRE_THAT(event, EventCommandNamesMatches(UnorderedRangeEquals(static_commands)));
                                REQUIRE_THAT(event,
                                             EventAttributeNamesMatches(UnorderedRangeEquals(static_attributes)));
                            }
                        }

                        AND_THEN("we execute a RestartServer command")
                        {
                            auto dserver = ctx.get_admin_proxy();

                            REQUIRE_NOTHROW(dserver->command_inout("RestartServer"));
                            std::this_thread::sleep_for(std::chrono::seconds(5));

                            THEN("we get an event")
                            {
                                auto event = cb.pop_next_event();
                                REQUIRE(event != std::nullopt);

                                REQUIRE_THAT(event, EventDeviceStarted(false));
                                REQUIRE_THAT(event, EventType(Tango::INTERFACE_CHANGE_EVENT));
                                REQUIRE_THAT(event, EventCommandNamesMatches(UnorderedRangeEquals(static_commands)));
                                REQUIRE_THAT(event,
                                             EventAttributeNamesMatches(UnorderedRangeEquals(static_attributes)));
                            }
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("DevIntrChangeEventData with not running DS")
{
    int idlver = GENERATE(TangoTest::idlversion(Tango::MIN_IDL_DEV_INTR));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"die", "DevInterEventDS", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());
        ctx.stop_server();

        AND_GIVEN("a subscription to the interface change event with stateless")
        {
            CallbackMockType cb;
            TangoTest::Subscription sub{device, Tango::INTERFACE_CHANGE_EVENT, &cb, true};

            THEN("we already got an interface change event")
            {
                auto event = cb.pop_next_event();
                REQUIRE(event != std::nullopt);

                using namespace TangoTest::Matchers;
                using namespace Catch::Matchers;

                REQUIRE_THAT(event, EventType(Tango::INTERFACE_CHANGE_EVENT));
                REQUIRE_THAT(event, EventErrorMatches(AllMatch(Reason(Tango::API_CantConnectToDevice))));
            }
        }
    }
}
