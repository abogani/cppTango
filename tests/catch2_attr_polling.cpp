#include <tango/tango.h>
#include <memory>
#include "utils/utils.h"

constexpr static Tango::DevBoolean k_initial_value = false;
constexpr static Tango::DevLong k_polling_period = 100; // 100 ms

template <class Base>
class AttrPollingCfg : public Base
{
  public:
    using Base::Base;

    ~AttrPollingCfg() override { }

    void init_device() override
    {
        value = k_initial_value;
    }

    void read_attribute(Tango::Attribute &att)
    {
        att.set_value(&value);
    }

    Tango::DevBoolean is_attr_polled(Tango::DevString attr)
    {
        return Tango::DeviceImpl::is_attribute_polled(attr);
    }

    Tango::DevLong get_attr_poll_period(Tango::DevString attr)
    {
        return Tango::DeviceImpl::get_attribute_poll_period(attr);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        using Attr = TangoTest::AutoAttr<&AttrPollingCfg::read_attribute>;

        attrs.push_back(new Attr("client_enabled_polling", Tango::DEV_BOOLEAN));
        attrs.push_back(new Attr("server_enabled_polling", Tango::DEV_BOOLEAN));
        attrs.back()->set_polling_period(k_polling_period);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&AttrPollingCfg::is_attr_polled>("IsAttrPolled"));
        cmds.push_back(new TangoTest::AutoCommand<&AttrPollingCfg::get_attr_poll_period>("AttrPollPeriod"));
    }

  private:
    Tango::DevBoolean value;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AttrPollingCfg, 4)

// TODO: Add case checking the client can enable polling when we have a database
SCENARIO("Attribute polling can be enabled")
{
    int idlver = GENERATE(range(4, 7));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_polling", "AttrPollingCfg", idlver};
        INFO(ctx.info());
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        std::string attr = GENERATE(as<std::string>(), "client_enabled_polling", "server_enabled_polling");
        bool setup_polling = attr.find("client") == 0;

        AND_GIVEN("an attribute " << (setup_polling ? "that we enable polling for" : "with polling already enabled"))
        {
            if(setup_polling)
            {
                REQUIRE_NOTHROW(device->poll_attribute(attr, k_polling_period));
            }

            THEN("the device proxy reports the attribute is polled")
            {
                REQUIRE(device->is_attribute_polled(attr));

                AND_THEN("the device server reports the attribute is polled")
                {
                    Tango::DeviceData in, out;
                    in << attr;
                    REQUIRE_NOTHROW(out = device->command_inout("IsAttrPolled", in));
                    REQUIRE_THAT(out, TangoTest::AnyLikeContains(true));
                }
            }

            THEN("the device proxy reports the correct polling period")
            {
                REQUIRE(device->get_attribute_poll_period(attr) == k_polling_period);

                AND_THEN("the device server reports the correct polling period")
                {
                    Tango::DeviceData in, out;
                    in << attr;
                    REQUIRE_NOTHROW(out = device->command_inout("AttrPollPeriod", in));
                    REQUIRE_THAT(out, TangoTest::AnyLikeContains(k_polling_period));
                }
            }
        }
    }
}

SCENARIO("Attribute polling period can be updated")
{
    int idlver = GENERATE(range(4, 7));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_polling", "AttrPollingCfg", idlver};
        INFO(ctx.info());
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute with polling already enabled")
        {
            std::string attr = "server_enabled_polling";

            WHEN("the device proxy increases the polling period")
            {
                REQUIRE_NOTHROW(device->poll_attribute(attr, 2 * k_polling_period));

                THEN("the device proxy reports the correct polling period")
                {
                    REQUIRE(device->get_attribute_poll_period(attr) == 2 * k_polling_period);

                    AND_THEN("the device server reports the correct polling period")
                    {
                        Tango::DeviceData in, out;
                        in << attr;
                        REQUIRE_NOTHROW(out = device->command_inout("AttrPollPeriod", in));
                        REQUIRE_THAT(out, TangoTest::AnyLikeContains(2 * k_polling_period));
                    }
                }
            }
        }
    }
}

SCENARIO("Attribute polling can be disabled")
{
    int idlver = GENERATE(range(4, 7));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_polling", "AttrPollingCfg", idlver};
        INFO(ctx.info());
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute with polling already enabled")
        {
            std::string attr = "server_enabled_polling";

            WHEN("the device proxy stops the polling")
            {
                REQUIRE_NOTHROW(device->stop_poll_attribute(attr));

                THEN("the device proxy reports the attribute is no longer polled")
                {
                    REQUIRE(!device->is_attribute_polled(attr));

                    AND_THEN("the device server reports the attribute is no longer polled")
                    {
                        Tango::DeviceData in, out;
                        in << attr;
                        REQUIRE_NOTHROW(out = device->command_inout("IsAttrPolled", in));
                        REQUIRE_THAT(out, TangoTest::AnyLikeContains(false));
                    }
                }
            }
        }
    }
}
