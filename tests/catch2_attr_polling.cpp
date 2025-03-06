#include "catch2_common.h"

constexpr static Tango::DevBoolean k_initial_value = false;
constexpr static Tango::DevBoolean k_new_value = true;
constexpr static Tango::DevLong k_polling_period = TANGO_TEST_CATCH2_DEFAULT_POLL_PERIOD;
constexpr static const char *k_test_reason = "Test_Reason";
constexpr static const char *k_a_helpful_desc = "A helpful description";

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
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_polling", "AttrPollingCfg", idlver};
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
                    using namespace TangoTest::Matchers;

                    Tango::DeviceData in, out;
                    in << attr;
                    REQUIRE_NOTHROW(out = device->command_inout("IsAttrPolled", in));
                    REQUIRE_THAT(out, AnyLikeContains(true));
                }
            }

            THEN("the device proxy reports the correct polling period")
            {
                REQUIRE(device->get_attribute_poll_period(attr) == k_polling_period);

                AND_THEN("the device server reports the correct polling period")
                {
                    using namespace TangoTest::Matchers;

                    Tango::DeviceData in, out;
                    in << attr;
                    REQUIRE_NOTHROW(out = device->command_inout("AttrPollPeriod", in));
                    REQUIRE_THAT(out, AnyLikeContains(k_polling_period));
                }
            }
        }
    }
}

SCENARIO("Attribute polling period can be updated")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_polling", "AttrPollingCfg", idlver};
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
                        using namespace TangoTest::Matchers;

                        Tango::DeviceData in, out;
                        in << attr;
                        REQUIRE_NOTHROW(out = device->command_inout("AttrPollPeriod", in));
                        REQUIRE_THAT(out, AnyLikeContains(2 * k_polling_period));
                    }
                }
            }
        }
    }
}

SCENARIO("Attribute polling can be disabled")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_polling", "AttrPollingCfg", idlver};
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
                        using namespace TangoTest::Matchers;

                        Tango::DeviceData in, out;
                        in << attr;
                        REQUIRE_NOTHROW(out = device->command_inout("IsAttrPolled", in));
                        REQUIRE_THAT(out, AnyLikeContains(false));
                    }
                }
            }
        }
    }
}

template <class Base>
class AttrPollingEvents : public Base
{
  public:
    using Base::Base;

    ~AttrPollingEvents() override { }

    void init_device() override
    {
        value = k_initial_value;
    }

    void read_attribute(Tango::Attribute &att)
    {
        if(throw_next)
        {
            throw_next = false;
            TANGO_THROW_EXCEPTION(k_test_reason, k_a_helpful_desc);
        }

        att.set_value(&value);
    }

    void update_value()
    {
        value = k_new_value;
    }

    void throw_on_next_read()
    {
        throw_next = true;
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        using Attr = TangoTest::AutoAttr<&AttrPollingEvents::read_attribute>;

        attrs.push_back(new Attr("attr", Tango::DEV_BOOLEAN));
        attrs.back()->set_polling_period(k_polling_period);
        attrs.push_back(new Attr("attr_no_polling", Tango::DEV_BOOLEAN));
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&AttrPollingEvents::throw_on_next_read>("ThrowOnNextRead"));
        cmds.push_back(new TangoTest::AutoCommand<&AttrPollingEvents::update_value>("UpdateValue"));
    }

  private:
    Tango::DevBoolean value;

    bool throw_next = false;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AttrPollingEvents, 4)

SCENARIO("Polled attributes generate change events")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_polling", "AttrPollingEvents", idlver};
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute with polling enabled")
        {
            std::string attr{"attr"};

            WHEN("we subscribe to change events for the attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(attr, Tango::CHANGE_EVENT, &callback));

                THEN("we receive some events with the initial value")
                {
                    require_initial_events(callback, k_initial_value);

                    AND_WHEN("we write to the attribute")
                    {
                        device->command_inout("UpdateValue");

                        THEN("we receive an event with the new value")
                        {
                            using namespace TangoTest::Matchers;

                            auto maybe_new_event = callback.pop_next_event();

                            REQUIRE(maybe_new_event != std::nullopt);
                            REQUIRE_THAT(maybe_new_event, EventValueMatches(AnyLikeContains(k_new_value)));

                            maybe_new_event = callback.pop_next_event(std::chrono::milliseconds{200});
                            REQUIRE(maybe_new_event == std::nullopt);
                        }
                    }

                    AND_WHEN("the attribute read throws an exception")
                    {
                        device->command_inout("ThrowOnNextRead");

                        THEN("we recieve an event with information about the exception")
                        {
                            using namespace Catch::Matchers;
                            using namespace TangoTest::Matchers;

                            auto maybe_ex_event = callback.pop_next_event();

                            REQUIRE(maybe_ex_event != std::nullopt);
                            REQUIRE_THAT(maybe_ex_event,
                                         EventErrorMatches(AllMatch(Reason(k_test_reason) &&
                                                                    DescriptionMatches(Equals(k_a_helpful_desc)))));

                            AND_THEN("we recieve a good event when the next read succeeds")
                            {
                                auto maybe_good_event = callback.pop_next_event();

                                REQUIRE(maybe_good_event != std::nullopt);
                                REQUIRE_THAT(maybe_good_event, EventValueMatches(AnyLikeContains(k_initial_value)));

                                maybe_good_event = callback.pop_next_event(std::chrono::milliseconds{200});
                                REQUIRE(maybe_good_event == std::nullopt);
                            }
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Subscribing to change events for an attribute with no polling fails")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"no_polling", "AttrPollingEvents", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();
        AND_GIVEN("an attribute with no polling")
        {
            std::string att{"attr_no_polling"};

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe with stateless=false to change events")
            {
                THEN("the subscription fails")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(device->subscribe_event(att, Tango::CHANGE_EVENT, &callback, false),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_AttributePollingNotStarted)));
                }
            }

            WHEN("we subscribe with stateless=true to change events")
            {
                THEN("the subscription succeeds")
                {
                    REQUIRE_NOTHROW(device->subscribe_event(att, Tango::CHANGE_EVENT, &callback, true));

                    AND_THEN("we receive an error event")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_initial_event = callback.pop_next_event();
                        REQUIRE(maybe_initial_event != std::nullopt);
                        REQUIRE_THAT(maybe_initial_event,
                                     EventErrorMatches(AnyMatch(Reason(Tango::API_AttributePollingNotStarted))));
                    }
                }
            }
        }
    }
}

SCENARIO("The attribute polling ring can be set")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device with attr_poll_ring_depth set")
    {
        TangoTest::Context ctx{"attr_polling",
                               "AttrPollingCfg",
                               idlver,
                               "TestServer/tests/1->attr_poll_ring_depth: server_enabled_polling,\\ 5\n"};
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        WHEN("we read the polling status")
        {
            auto *poll_status = device->polling_status();
            REQUIRE(poll_status->size() == 1);
            const std::string polling_item = poll_status->at(0);
            delete poll_status;
            THEN("The polling ring depth is indeed set")
            {
                if(polling_item.find("name = server_enabled_polling") != std::string::npos)
                {
                    using Catch::Matchers::ContainsSubstring;
                    REQUIRE_THAT(polling_item, ContainsSubstring("Polling ring buffer depth = 5"));
                }
            }
        }
    }
}
