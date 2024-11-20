#include <tango/tango.h>

#include <memory>

#include "utils/utils.h"

#include <tango/internal/stl_corba_helpers.h>

static constexpr double CMD_RET_VOID_RETURN_VALUE = 42.0;
static constexpr double CMD_VOID_ARG_TEST_VALUE = 84.0;
static constexpr double CMD_RET_ARG_TEST_VALUE = 168.0;
static constexpr Tango::DevLong CMD_LONG_TEST_VALUE = 4711;

// Test device class
template <class Base>
class AutoCmdDev : public Base
{
  public:
    using Base::Base;

    ~AutoCmdDev() override { }

    void init_device() override
    {
        cmd_run = false;
        value = 0;
    }

    // a command that doesn't accept arguments and doesn't return anything
    void cmd_void_void()
    {
        cmd_run = true;
    }

    // a command that returns a value and doesn't accept arguments
    double cmd_ret_void()
    {
        return CMD_RET_VOID_RETURN_VALUE;
    }

    // a command that accepts an argument and doesn't return anything
    void cmd_void_arg(double v)
    {
        value = v;
    }

    // a command that accepts an argument and returns a value
    double cmd_ret_arg(double v)
    {
        return v;
    }

    void cmd_void_long_array(const Tango::DevVarLongArray &in)
    {
        long_value = in[0];
    }

    Tango::DevLong cmd_ret_long_array(const Tango::DevVarLongArray &in)
    {
        return in[0] + 1;
    }

    // a flag attribute for void-void command
    void read_cmd_run(Tango::Attribute &att)
    {
        att.set_value_date_quality(&cmd_run, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    // value attribute for void-arg command
    void read_value(Tango::Attribute &att)
    {
        att.set_value_date_quality(&value, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    // long_value attribute for cmd_void_long_array/cmd_ret_long_array
    void read_long_value(Tango::Attribute &att)
    {
        att.set_value_date_quality(&long_value, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&AutoCmdDev::read_cmd_run>("cmd_run", Tango::DEV_BOOLEAN));
        attrs.push_back(new TangoTest::AutoAttr<&AutoCmdDev::read_value>("value", Tango::DEV_DOUBLE));
        attrs.push_back(new TangoTest::AutoAttr<&AutoCmdDev::read_long_value>("long_value", Tango::DEV_LONG));
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&AutoCmdDev::cmd_void_void>("void_void"));
        cmds.push_back(new TangoTest::AutoCommand<&AutoCmdDev::cmd_ret_void>("ret_void"));
        cmds.push_back(new TangoTest::AutoCommand<&AutoCmdDev::cmd_void_arg>("void_arg"));
        cmds.push_back(new TangoTest::AutoCommand<&AutoCmdDev::cmd_ret_arg>("ret_arg"));
        cmds.push_back(new TangoTest::AutoCommand<&AutoCmdDev::cmd_void_long_array>("cmd_void_long_array"));
        cmds.push_back(new TangoTest::AutoCommand<&AutoCmdDev::cmd_ret_long_array>("cmd_ret_long_array"));
    }

  private:
    Tango::DevBoolean cmd_run;
    Tango::DevDouble value;
    Tango::DevLong long_value;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AutoCmdDev, 3)

SCENARIO("AutoCommand executes correctly")
{
    int idlver = GENERATE(TangoTest::idlversion(3));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"auto_command", "AutoCmdDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a no-arg, no-return command name and flag attribute name")
        {
            std::string cmd{"void_void"};
            std::string att{"cmd_run"};

            WHEN("we execute the command")
            {
                REQUIRE_NOTHROW(device->command_inout(cmd));

                THEN("we see that the flag was set")
                {
                    Tango::DeviceAttribute da;
                    REQUIRE_NOTHROW(da = device->read_attribute(att));
                    bool flag_value;
                    da >> flag_value;
                    REQUIRE(flag_value == true);
                }
            }
        }

        AND_GIVEN("a no-arg command that returns a value")
        {
            std::string cmd{"ret_void"};

            WHEN("we execute the command")
            {
                Tango::DeviceData dd;
                REQUIRE_NOTHROW(dd = device->command_inout(cmd));

                THEN("we get back the expected value")
                {
                    double r;
                    dd >> r;
                    REQUIRE(r == CMD_RET_VOID_RETURN_VALUE);
                }
            }
        }

        AND_GIVEN("a no-return command that accepts an argument and a value attribute")
        {
            std::string cmd{"void_arg"};
            std::string att{"value"};

            WHEN("we execute the command")
            {
                Tango::DeviceData dd;
                dd << CMD_VOID_ARG_TEST_VALUE;
                REQUIRE_NOTHROW(device->command_inout(cmd, dd));

                THEN("we get back the expected value")
                {
                    Tango::DeviceAttribute da;
                    REQUIRE_NOTHROW(da = device->read_attribute(att));
                    double att_value;
                    da >> att_value;
                    REQUIRE(att_value == CMD_VOID_ARG_TEST_VALUE);
                }
            }
        }

        AND_GIVEN("a command that returns a value passed as argument")
        {
            std::string cmd{"ret_arg"};

            WHEN("we execute the command")
            {
                Tango::DeviceData in;
                Tango::DeviceData out;
                in << CMD_RET_ARG_TEST_VALUE;
                REQUIRE_NOTHROW(out = device->command_inout(cmd, in));

                THEN("we get back the expected value")
                {
                    double r;
                    out >> r;
                    REQUIRE(r == CMD_RET_ARG_TEST_VALUE);
                }
            }
        }
        AND_GIVEN("a command with no-return but accepting a long array")
        {
            std::string cmd{"cmd_void_long_array"};

            WHEN("we execute the command")
            {
                Tango::DeviceData dd;
                Tango::DevVarLongArray_var arr = new Tango::DevVarLongArray();
                arr->length(1);
                arr[0] = 42;
                dd << arr;

                REQUIRE_NOTHROW(device->command_inout(cmd, dd));

                std::string att{"long_value"};
                Tango::DeviceAttribute da;
                REQUIRE_NOTHROW(da = device->read_attribute(att));
                Tango::DevLong val;
                da >> val;

                REQUIRE(val == 42);
            }
        }

        AND_GIVEN("a command with no-return but accepting a long array")
        {
            std::string cmd{"cmd_void_long_array"};

            WHEN("we execute the command")
            {
                Tango::DeviceData in;
                Tango::DevVarLongArray_var arr = new Tango::DevVarLongArray();
                arr->length(1);
                arr[0] = CMD_LONG_TEST_VALUE;
                in << arr;

                REQUIRE_NOTHROW(device->command_inout(cmd, in));

                std::string att{"long_value"};
                Tango::DeviceAttribute da;
                REQUIRE_NOTHROW(da = device->read_attribute(att));
                Tango::DevLong val;
                da >> val;

                REQUIRE(val == CMD_LONG_TEST_VALUE);
            }
        }
        AND_GIVEN("a command with long return and accepting a long array")
        {
            std::string cmd{"cmd_ret_long_array"};

            WHEN("we execute the command")
            {
                Tango::DeviceData in;
                Tango::DeviceData out;
                Tango::DevVarLongArray_var in_arr = new Tango::DevVarLongArray();
                in_arr->length(1);
                in_arr[0] = CMD_LONG_TEST_VALUE;
                in << in_arr;

                REQUIRE_NOTHROW(out = device->command_inout(cmd, in));

                Tango::DevLong val;
                out >> val;
                REQUIRE(val == CMD_LONG_TEST_VALUE + 1);
            }
        }
    }
}
