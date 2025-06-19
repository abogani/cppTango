#include "catch2_common.h"

static constexpr double k_initial_value{1.1234};

template <class Base>
class AttrConfEventData : public Base
{
  public:
    using Base::Base;

    void init_device() override { }

    void read_attr(Tango::Attribute &att) override
    {
        att.set_value(&attr_dq_double);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&AttrConfEventData::read_attr>("double_attr", Tango::DEV_DOUBLE));
    }

  private:
    Tango::DevDouble attr_dq_double{k_initial_value};
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AttrConfEventData, 1)

SCENARIO("Setting AttributeConfig works without database")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"double_attr", "AttrConfEventData", idlver};
        auto device = ctx.get_proxy();
        const std::string reset_value = "Not specified";

        REQUIRE(idlver == device->get_idl_version());

        THEN("we can change the attribute configuration")
        {
            using namespace TangoTest::Matchers;
            std::string attr{"double_attr"};
            auto ai = device->attribute_query(attr);
            ai.events.ch_event.abs_change = "33333";
            ai.events.ch_event.rel_change = "99.99";

            Tango::AttributeInfoListEx ail;
            ail.push_back(ai);
            REQUIRE_NOTHROW(device->set_attribute_config(ail));
        }
    }
}

SCENARIO("AttributeConfig returns correct data")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"double_attr", "AttrConfEventData", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();
        const std::string reset_value = "Not specified";

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute")
        {
            std::string attr{"double_attr"};

            THEN("we change the attribute configuration")
            {
                {
                    auto ai = device->attribute_query(attr);
                    ai.events.ch_event.abs_change = "33333";
                    ai.events.ch_event.rel_change = "99.99";

                    Tango::AttributeInfoListEx ail;
                    ail.push_back(ai);
                    device->set_attribute_config(ail);
                }

                WHEN("we subscribe to configuration change events for the attribute")
                {
                    TangoTest::CallbackMock<Tango::AttrConfEventData> callback;
                    const std::vector<std::string> filters;
                    TangoTest::Subscription sub{device, attr, Tango::ATTR_CONF_EVENT, &callback, filters};

                    require_initial_events(callback);

                    AND_THEN("change the configuration again")
                    {
                        auto ai = device->attribute_query(attr);
                        ai.events.ch_event.abs_change = reset_value;
                        ai.events.ch_event.rel_change = reset_value;

                        Tango::AttributeInfoListEx ail;
                        ail.push_back(ai);
                        device->set_attribute_config(ail);

                        THEN("we receive an event with the reset rel/abs change")
                        {
                            using namespace TangoTest::Matchers;

                            auto maybe_new_event = callback.pop_next_event();

                            REQUIRE(maybe_new_event != std::nullopt);
                            REQUIRE(maybe_new_event->attr_conf != nullptr);
                            REQUIRE_THAT(maybe_new_event, EventType(Tango::ATTR_CONF_EVENT));
                            REQUIRE(maybe_new_event->attr_conf->events.ch_event.abs_change == reset_value);
                            REQUIRE(maybe_new_event->attr_conf->events.ch_event.rel_change == reset_value);

                            AND_THEN("these are equal to the queried ones")
                            {
                                auto ai = device->attribute_query(attr);
                                REQUIRE(ai.events.ch_event.abs_change == reset_value);
                                REQUIRE(ai.events.ch_event.rel_change == reset_value);
                            }
                        }
                    }
                }
            }
        }
    }
}
