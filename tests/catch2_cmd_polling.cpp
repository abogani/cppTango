#include "catch2_common.h"

constexpr static Tango::DevLong k_polling_period = 100; // 100 ms

template <class Base>
class CmdPollingCfg : public Base
{
  public:
    using Base::Base;

    ~CmdPollingCfg() override { }

    void init_device() override { }

    Tango::DevBoolean is_cmd_polled(Tango::DevString attr)
    {
        return Tango::DeviceImpl::is_command_polled(attr);
    }

    Tango::DevLong get_cmd_poll_period(Tango::DevString attr)
    {
        return Tango::DeviceImpl::get_command_poll_period(attr);
    }

    void some_cmd() { }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&CmdPollingCfg::some_cmd>("ClientEnabledPolling"));
        cmds.push_back(new TangoTest::AutoCommand<&CmdPollingCfg::some_cmd>("ServerEnabledPolling"));
        cmds.back()->set_polling_period(k_polling_period);
        cmds.push_back(new TangoTest::AutoCommand<&CmdPollingCfg::is_cmd_polled>("IsCmdPolled"));
        cmds.push_back(new TangoTest::AutoCommand<&CmdPollingCfg::get_cmd_poll_period>("CmdPollPeriod"));
    }
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(CmdPollingCfg, 4)

// TODO: Add case checking the client can enable polling when we have a database
SCENARIO("Command polling can be enabled")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"cmd_polling", "CmdPollingCfg", idlver};
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        std::string cmd = GENERATE(as<std::string>(), "ClientEnabledPolling", "ServerEnabledPolling");
        bool setup_polling = cmd.find("Client") == 0;

        AND_GIVEN("a command " << (setup_polling ? "that we enable polling for" : "with polling already enabled"))
        {
            if(setup_polling)
            {
                REQUIRE_NOTHROW(device->poll_command(cmd, k_polling_period));
            }

            THEN("the device proxy reports the command is polled")
            {
                REQUIRE(device->is_command_polled(cmd));

                AND_THEN("the device server reports the command is polled")
                {
                    Tango::DeviceData in, out;
                    in << cmd;
                    REQUIRE_NOTHROW(out = device->command_inout("IsCmdPolled", in));
                    REQUIRE_THAT(out, TangoTest::AnyLikeContains(true));
                }
            }

            THEN("the device proxy reports the correct polling period")
            {
                REQUIRE(device->get_command_poll_period(cmd) == k_polling_period);

                AND_THEN("the device server reports the correct polling period")
                {
                    Tango::DeviceData in, out;
                    in << cmd;
                    REQUIRE_NOTHROW(out = device->command_inout("CmdPollPeriod", in));
                    REQUIRE_THAT(out, TangoTest::AnyLikeContains(k_polling_period));
                }
            }
        }
    }
}

SCENARIO("Command polling period can be updated")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"cmd_polling", "CmdPollingCfg", idlver};
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a command with polling already enabled")
        {
            std::string cmd = "ServerEnabledPolling";

            WHEN("the device proxy increases the polling period")
            {
                REQUIRE_NOTHROW(device->poll_command(cmd, 2 * k_polling_period));

                THEN("the device proxy reports the correct polling period")
                {
                    REQUIRE(device->get_command_poll_period(cmd) == 2 * k_polling_period);

                    AND_THEN("the device server reports the correct polling period")
                    {
                        Tango::DeviceData in, out;
                        in << cmd;
                        REQUIRE_NOTHROW(out = device->command_inout("CmdPollPeriod", in));
                        REQUIRE_THAT(out, TangoTest::AnyLikeContains(2 * k_polling_period));
                    }
                }
            }
        }
    }
}

SCENARIO("Command polling can be disabled")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"cmd_polling", "CmdPollingCfg", idlver};
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a cmd with polling already enabled")
        {
            std::string cmd = "ServerEnabledPolling";

            WHEN("the device proxy stops the polling")
            {
                REQUIRE_NOTHROW(device->stop_poll_command(cmd));

                THEN("the device proxy reports the command is no longer polled")
                {
                    REQUIRE(!device->is_command_polled(cmd));

                    AND_THEN("the device server reports the command is no longer polled")
                    {
                        Tango::DeviceData in, out;
                        in << cmd;
                        REQUIRE_NOTHROW(out = device->command_inout("IsCmdPolled", in));
                        REQUIRE_THAT(out, TangoTest::AnyLikeContains(false));
                    }
                }
            }
        }
    }
}
