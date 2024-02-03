#include <tango/tango.h>
#include <memory>
#include "utils/utils.h"

static constexpr double ATTR_INIT_VALUE = 0.0;
static const double ATTR_NAN_VALUE = std::nan("nan");

// Test device class
template <class Base>
class ChangeEventOnNanDev : public Base
{
  public:
    using Base::Base;

    ~ChangeEventOnNanDev() override { }

    void init_device() override
    {
        attr_abs_value = ATTR_INIT_VALUE;
        attr_rel_value = ATTR_INIT_VALUE;
    }

    void set_abs_nan(void)
    {
        attr_abs_value = ATTR_NAN_VALUE;
    }

    void set_rel_nan(void)
    {
        attr_rel_value = ATTR_NAN_VALUE;
    }

    void unset_abs_nan(void)
    {
        attr_abs_value = ATTR_INIT_VALUE;
    }

    void unset_rel_nan(void)
    {
        attr_rel_value = ATTR_INIT_VALUE;
    }

    void read_abs(Tango::Attribute &att)
    {
        att.set_value_date_quality(&attr_abs_value, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    void read_rel(Tango::Attribute &att)
    {
        att.set_value_date_quality(&attr_rel_value, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        // attribute with absolute change
        auto attr_abs = new TangoTest::AutoAttr<&ChangeEventOnNanDev::read_abs>("attr_abs", Tango::DEV_DOUBLE);
        Tango::UserDefaultAttrProp abs_props;
        abs_props.set_event_abs_change("0.01");
        attr_abs->set_default_properties(abs_props);
        attr_abs->set_polling_period(100);
        attrs.push_back(attr_abs);

        // attribute with relative change
        auto attr_rel = new TangoTest::AutoAttr<&ChangeEventOnNanDev::read_rel>("attr_rel", Tango::DEV_DOUBLE);
        Tango::UserDefaultAttrProp rel_props;
        rel_props.set_event_rel_change("0.01");
        attr_rel->set_default_properties(rel_props);
        attr_rel->set_polling_period(100);
        attrs.push_back(attr_rel);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev::set_abs_nan>("set_abs_nan"));
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev::unset_abs_nan>("unset_abs_nan"));
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev::set_rel_nan>("set_rel_nan"));
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev::unset_rel_nan>("unset_rel_nan"));
    }

  private:
    Tango::DevDouble attr_abs_value;
    Tango::DevDouble attr_rel_value;
};

// Test event callback which can test certain things about last received event
class EvCb : public Tango::CallBack
{
  public:
    EvCb() :
        event_received(false)
    {
    }

    void push_event(Tango::EventData *ev)
    {
        event_received = true;
        last_event_type = ev->event;
        *(ev->attr_value) >> last_event_value;
        last_event_quality = ev->attr_value->quality;
        std::cout << "push: event_received=" << event_received << "; last_event_type=" << last_event_type
                  << "; last_event_value=" << last_event_value << "; last_event_quality=" << last_event_quality << "\n";
    }

    bool test_last_event(std::string expected_event_type,
                         Tango::DevDouble expected_value,
                         Tango::AttrQuality expected_quality)
    {
        std::cout << "test: event_received=" << event_received << "; last_event_type=" << last_event_type
                  << "; last_event_value=" << last_event_value << "; last_event_quality=" << last_event_quality << "\n";
        if(event_received)
        {
            if(last_event_type == expected_event_type && last_event_value == expected_value &&
               last_event_quality == expected_quality)
            {
                event_received = false;
                return true;
            }
        }
        return false;
    }

  private:
    bool event_received;
    std::string last_event_type;
    Tango::DevDouble last_event_value;
    Tango::AttrQuality last_event_quality;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(ChangeEventOnNanDev, 4)

SCENARIO("Change events are generated on NaN with absolute change")
{
    int idlver = GENERATE(range(4, 7));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"change_event_on_nan", "ChangeEventOnNanDev", idlver};
        INFO(ctx.info());
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute with absolute change")
        {
            std::string att{"attr_abs"};

            REQUIRE(device->is_attribute_polled(att) == true);

            AND_GIVEN("a change event subscription")
            {
                EvCb callback;
                int evid;
                REQUIRE_NOTHROW(evid = device->subscribe_event(att, Tango::CHANGE_EVENT, &callback));

                WHEN("we set the attribute value to NaN")
                {
                    REQUIRE_NOTHROW(device->command_inout("set_abs_nan"));

                    THEN("a change event is generated")
                    {
                        REQUIRE(callback.test_last_event("change", ATTR_NAN_VALUE, Tango::ATTR_VALID));
                    }
                }

                WHEN("we unset the attribute value from NaN")
                {
                    REQUIRE_NOTHROW(device->command_inout("unset_abs_nan"));

                    THEN("a change event is generated")
                    {
                        REQUIRE(callback.test_last_event("change", ATTR_INIT_VALUE, Tango::ATTR_VALID));
                    }
                }
            }
        }
    }
}

SCENARIO("Change events are generated on NaN with relative change")
{
    int idlver = GENERATE(range(4, 7));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"change_event_on_nan", "ChangeEventOnNanDev", idlver};
        INFO(ctx.info());
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute with relative change")
        {
            std::string att{"attr_rel"};

            REQUIRE(device->is_attribute_polled(att) == true);

            AND_GIVEN("a change event subscription")
            {
                EvCb callback;
                int evid;
                REQUIRE_NOTHROW(evid = device->subscribe_event(att, Tango::CHANGE_EVENT, &callback));

                WHEN("we set the attribute value to NaN")
                {
                    REQUIRE_NOTHROW(device->command_inout("set_rel_nan"));

                    THEN("a change event is generated")
                    {
                        REQUIRE(callback.test_last_event("change", ATTR_NAN_VALUE, Tango::ATTR_VALID));
                    }
                }

                WHEN("we unset the attribute value from NaN")
                {
                    REQUIRE_NOTHROW(device->command_inout("unset_rel_nan"));

                    THEN("a change event is generated")
                    {
                        REQUIRE(callback.test_last_event("change", ATTR_INIT_VALUE, Tango::ATTR_VALID));
                    }
                }
            }
        }
    }
}
