#include "catch2_common.h"

SCENARIO("Command list can be retrieved")
{
    GIVEN("a device proxy to an DServer device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask for the command list from the device proxy")
        {
            std::unique_ptr<Tango::CommandInfoList> ptr;
            REQUIRE_NOTHROW(ptr = std::unique_ptr<Tango::CommandInfoList>{dserver->command_list_query()});

            THEN("we get the expected number of DServer commands")
            {
                using namespace Catch::Matchers;

                CHECK_THAT(*ptr, SizeIs(34));

                auto has_info_for = [](std::string name)
                {
                    return AnyMatch(Predicate<Tango::CommandInfo>(
                        [name](const auto &cmd_inf) { return cmd_inf.cmd_name == name; }, "has name " + name));
                };

                CHECK_THAT(*ptr, has_info_for("AddLoggingTarget"));
                CHECK_THAT(*ptr, has_info_for("AddObjPolling"));
                CHECK_THAT(*ptr, has_info_for("DevLockStatus"));
                CHECK_THAT(*ptr, has_info_for("DevPollStatus"));
                CHECK_THAT(*ptr, has_info_for("DevRestart"));
                CHECK_THAT(*ptr, has_info_for("EnableEventSystemPerfMon"));
                CHECK_THAT(*ptr, has_info_for("EventConfirmSubscription"));
                CHECK_THAT(*ptr, has_info_for("EventSubscriptionChange"));
                CHECK_THAT(*ptr, has_info_for("GetLoggingLevel"));
                CHECK_THAT(*ptr, has_info_for("Init"));
                CHECK_THAT(*ptr, has_info_for("Kill"));
                CHECK_THAT(*ptr, has_info_for("LockDevice"));
                CHECK_THAT(*ptr, has_info_for("PolledDevice"));
                CHECK_THAT(*ptr, has_info_for("QueryClass"));
                CHECK_THAT(*ptr, has_info_for("QueryDevice"));
                CHECK_THAT(*ptr, has_info_for("QueryEventSystem"));
                CHECK_THAT(*ptr, has_info_for("QuerySubDevice"));
                CHECK_THAT(*ptr, has_info_for("QueryWizardClassProperty"));
                CHECK_THAT(*ptr, has_info_for("QueryWizardDevProperty"));
                CHECK_THAT(*ptr, has_info_for("RemObjPolling"));
                CHECK_THAT(*ptr, has_info_for("RemoveLoggingTarget"));
                CHECK_THAT(*ptr, has_info_for("RestartServer"));
                CHECK_THAT(*ptr, has_info_for("StartLogging"));
                CHECK_THAT(*ptr, has_info_for("StartPolling"));
                CHECK_THAT(*ptr, has_info_for("State"));
                CHECK_THAT(*ptr, has_info_for("Status"));
                CHECK_THAT(*ptr, has_info_for("StopLogging"));
                CHECK_THAT(*ptr, has_info_for("StopPolling"));
                CHECK_THAT(*ptr, has_info_for("UnLockDevice"));
                CHECK_THAT(*ptr, has_info_for("UpdObjPollingPeriod"));
                CHECK_THAT(*ptr, has_info_for("ZmqEventSubscriptionChange"));
            }
        }
    }
}

SCENARIO("Querying for invalid command")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto device = ctx.get_proxy();

        THEN("we get an exception when asking for an invalid command")
        {
            using namespace TangoTest::Matchers;

            REQUIRE_THROWS_MATCHES(device->command_query("DevToto"),
                                   Tango::DevFailed,
                                   FirstErrorMatches(Reason(Tango::API_CommandNotFound) && Severity(Tango::ERR)));
        }
    }
}

SCENARIO("Status command can be queried from normal device")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto device = ctx.get_proxy();

        WHEN("we ask the device proxy about the Status command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = device->command_query("Status"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "Status");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_STRING);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Device status");
            }
        }
    }
}

SCENARIO("AddLoggingTarget command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the AddLoggingTarget command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("AddLoggingTarget"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "AddLoggingTarget");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Str[i]=Device-name. Str[i+1]=Target-type::Target-name");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("AddObjPolling command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the AddObjPolling command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("AddObjPolling"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "AddObjPolling");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_LONGSTRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc ==
                      "Lg[0]=Upd period. Str[0]=Device name. Str[1]=Object type. Str[2]=Object name");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("DevLockStatus command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the DevLockStatus command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("DevLockStatus"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "DevLockStatus");
                CHECK(cmd_inf.in_type == Tango::DEV_STRING);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_LONGSTRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Device name");
                CHECK(cmd_inf.out_type_desc == "Device locking status");
            }
        }
    }
}

SCENARIO("DevPollStatus command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the DevPollStatus command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("DevPollStatus"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "DevPollStatus");
                CHECK(cmd_inf.in_type == Tango::DEV_STRING);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Device name");
                CHECK(cmd_inf.out_type_desc == "Device polling status");
            }
        }
    }
}

SCENARIO("DevRestart command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the DevRestart command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("DevRestart"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "DevRestart");
                CHECK(cmd_inf.in_type == Tango::DEV_STRING);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Device name");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("EnableEventSystemPerfMon command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the EnableEventSystemPerfMon command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("EnableEventSystemPerfMon"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "EnableEventSystemPerfMon");
                CHECK(cmd_inf.in_type == Tango::DEV_BOOLEAN);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Enable or disable the collection of performance samples for events");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("EventConfirmSubscription command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the EventConfirmSubscription command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("EventConfirmSubscription"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "EventConfirmSubscription");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc ==
                      "Str[0] = dev1 name, Str[1] = att1 name, Str[2] = event name, Str[3] = dev2 name, Str[4] = "
                      "att2 name, Str[5] = event name,...");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("EventSubscriptionChange command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the EventSubscriptionChange command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("EventSubscriptionChange"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "EventSubscriptionChange");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEV_LONG);
                CHECK(cmd_inf.in_type_desc == "Event consumer wants to subscribe to");
                CHECK(cmd_inf.out_type_desc == "Tango lib release");
            }
        }
    }
}

SCENARIO("GetLoggingLevel command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the GetLoggingLevel command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("GetLoggingLevel"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "GetLoggingLevel");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_LONGSTRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Device list");
                CHECK(cmd_inf.out_type_desc == "Lg[i]=Logging Level. Str[i]=Device name.");
            }
        }
    }
}

SCENARIO("Init command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the Init command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("Init"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "Init");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("Kill command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the Kill command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("Kill"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "Kill");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("LockDevice command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the LockDevice command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("LockDevice"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "LockDevice");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_LONGSTRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Str[0] = Device name. Lg[0] = Lock validity");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("PolledDevice command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the PolledDevice command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("PolledDevice"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "PolledDevice");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Polled device name list");
            }
        }
    }
}

SCENARIO("QueryClass command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the QueryClass command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("QueryClass"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "QueryClass");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Device server class(es) list");
            }
        }
    }
}

SCENARIO("QueryDevice command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the QueryDevice command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("QueryDevice"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "QueryDevice");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Device server device(s) list");
            }
        }
    }
}

SCENARIO("QueryEventSystem command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the QueryEventSystem command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("QueryEventSystem"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "QueryEventSystem");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_STRING);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "JSON object with information about the event system");
            }
        }
    }
}

SCENARIO("QuerySubDevice command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the QuerySubDevice command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("QuerySubDevice"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "QuerySubDevice");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Device server sub device(s) list");
            }
        }
    }
}

SCENARIO("QueryWizardClassProperty command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the QueryWizardClassProperty command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("QueryWizardClassProperty"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "QueryWizardClassProperty");
                CHECK(cmd_inf.in_type == Tango::DEV_STRING);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Class name");
                CHECK(cmd_inf.out_type_desc == "Class property list (name - description and default value)");
            }
        }
    }
}

SCENARIO("QueryWizardDevProperty command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the QueryWizardDevProperty command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("QueryWizardDevProperty"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "QueryWizardDevProperty");
                CHECK(cmd_inf.in_type == Tango::DEV_STRING);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Class name");
                CHECK(cmd_inf.out_type_desc == "Device property list (name - description and default value)");
            }
        }
    }
}

SCENARIO("RemObjPolling command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the RemObjPolling command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("RemObjPolling"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "RemObjPolling");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Str[0]=Device name. Str[1]=Object type. Str[2]=Object name");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("RemoveLoggingTarget command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the RemoveLoggingTarget command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("RemoveLoggingTarget"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "RemoveLoggingTarget");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Str[i]=Device-name. Str[i+1]=Target-type::Target-name");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("RestartServer command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the RestartServer command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("RestartServer"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "RestartServer");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("StartLogging command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the StartLogging command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("StartLogging"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "StartLogging");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("StartPolling command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the StartPolling command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("StartPolling"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "StartPolling");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("State command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the State command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("State"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "State");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_STATE);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Device state");
            }
        }
    }
}

SCENARIO("Status command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the Status command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("Status"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "Status");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_STRING);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Device status");
            }
        }
    }
}

SCENARIO("StopLogging command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the StopLogging command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("StopLogging"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "StopLogging");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("StopPolling command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the StopPolling command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("StopPolling"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "StopPolling");
                CHECK(cmd_inf.in_type == Tango::DEV_VOID);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc == "Uninitialised");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("UnLockDevice command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the UnLockDevice command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("UnLockDevice"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "UnLockDevice");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_LONGSTRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEV_LONG);
                CHECK(cmd_inf.in_type_desc == "Str[x] = Device name(s). Lg[0] = Force flag");
                CHECK(cmd_inf.out_type_desc == "Device global lock counter");
            }
        }
    }
}

SCENARIO("UpdObjPollingPeriod command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the UpdObjPollingPeriod command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("UpdObjPollingPeriod"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "UpdObjPollingPeriod");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_LONGSTRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEV_VOID);
                CHECK(cmd_inf.in_type_desc ==
                      "Lg[0]=Upd period. Str[0]=Device name. Str[1]=Object type. Str[2]=Object name");
                CHECK(cmd_inf.out_type_desc == "Uninitialised");
            }
        }
    }
}

SCENARIO("ZmqEventSubscriptionChange command can be queried")
{
    GIVEN("a device proxy to a device")
    {
        TangoTest::Context ctx{"empty", "Empty"};
        auto dserver = ctx.get_admin_proxy();

        WHEN("we ask the device proxy about the ZmqEventSubscriptionChange command")
        {
            Tango::CommandInfo cmd_inf;
            REQUIRE_NOTHROW(cmd_inf = dserver->command_query("ZmqEventSubscriptionChange"));

            THEN("we get the expected information")
            {
                using namespace Catch::Matchers;
                CHECK(cmd_inf.cmd_name == "ZmqEventSubscriptionChange");
                CHECK(cmd_inf.in_type == Tango::DEVVAR_STRINGARRAY);
                CHECK(cmd_inf.out_type == Tango::DEVVAR_LONGSTRINGARRAY);
                CHECK(cmd_inf.in_type_desc == "Event consumer wants to subscribe to.\n"
                                              "device name, attribute/pipe name, action (\"subscribe\"), event name, "
                                              "<Tango client IDL version>\"\n"
                                              "event name can take the following values:\n"
                                              "    \"change\",\n"
                                              "    \"alarm\",\n"
                                              "    \"periodic\",\n"
                                              "    \"archive\",\n"
                                              "    \"user_event\",\n"
                                              "    \"attr_conf\",\n"
                                              "    \"data_ready\",\n"
                                              "    \"intr_change\",\n"
                                              "    \"pipe\"\n"
                                              "\"info\" can also be used as single parameter to retrieve information "
                                              "about the heartbeat and event pub "
                                              "endpoints.");
                CHECK(cmd_inf.out_type_desc ==
                      "Str[0] = Heartbeat pub endpoint - Str[1] = Event pub endpoint\n"
                      "...\n"
                      "Str[n] = Alternate Heartbeat pub endpoint - Str[n+1] = Alternate Event pub endpoint\n"
                      "Str[n+1] = event name used by this server as zmq topic to send events\n"
                      "Str[n+2] = channel name used by this server to send heartbeat events\n"
                      "Lg[0] = Tango lib release - Lg[1] = Device IDL release\n"
                      "Lg[2] = Subscriber HWM - Lg[3] = Multicast rate\n"
                      "Lg[4] = Multicast IVL - Lg[5] = ZMQ release");
            }
        }
    }
}
