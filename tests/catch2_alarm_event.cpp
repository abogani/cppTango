#include <tango/tango.h>

#include <memory>

#include "utils/utils.h"

static constexpr double ATTR_INIT_VALUE = 0.0;
static constexpr double ATTR_MIN_ALARM = -1.0;
static constexpr double ATTR_MAX_ALARM = 1.0;
static constexpr double ATTR_PUSH_VALUE = 10.0;

// Test device class
template <class Base>
class AlarmEventDev : public Base
{
  public:
    using Base::Base;

    ~AlarmEventDev() override { }

    void init_device() override
    {
        attr_value = ATTR_INIT_VALUE;
        attr_quality = Tango::ATTR_VALID;
    }

    void set_alarm()
    {
        attr_quality = Tango::ATTR_ALARM;
    }

    void set_valid()
    {
        attr_quality = Tango::ATTR_VALID;
    }

    void push_alarm()
    {
        Tango::DevDouble v{ATTR_PUSH_VALUE};
        this->push_alarm_event("attr_push", &v);
    }

    void read_attribute(Tango::Attribute &att)
    {
        att.set_value_date_quality(&attr_value, std::chrono::system_clock::now(), attr_quality);
    }

    void write_attribute(Tango::WAttribute &att)
    {
        att.get_write_value(attr_value);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        auto attr_test = new TangoTest::AutoAttr<&AlarmEventDev::read_attribute, &AlarmEventDev::write_attribute>(
            "attr_test", Tango::DEV_DOUBLE);
        // TODO: for now polling has to be enabled by server
        attr_test->set_polling_period(100);
        attrs.push_back(attr_test);

        // attribute which pushes alarm events from code
        auto attr_push = new TangoTest::AutoAttr<&AlarmEventDev::read_attribute, &AlarmEventDev::write_attribute>(
            "attr_push", Tango::DEV_DOUBLE);
        attr_push->set_alarm_event(true, false);
        attrs.push_back(attr_push);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::set_alarm>("set_alarm"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::set_valid>("set_valid"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::push_alarm>("push_alarm"));
    }

  private:
    Tango::DevDouble attr_value;
    Tango::AttrQuality attr_quality;
};

// Test event callback
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

// Alarm event is supported from IDL6 onwards
TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AlarmEventDev, 6)

SCENARIO("Attribute alarm range triggers ALARM_EVENT")
{
    int idlver = GENERATE(range(6, 7));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute name and configuration")
        {
            std::string att{"attr_test"};

            // configure min and max alarms
            Tango::AttributeInfo ai;
            Tango::AttributeInfoList ai_list;
            REQUIRE_NOTHROW(ai = device->get_attribute_config(att));
            ai.min_alarm = std::to_string(ATTR_MIN_ALARM);
            ai.max_alarm = std::to_string(ATTR_MAX_ALARM);
            ai_list.push_back(ai);
            REQUIRE_NOTHROW(device->set_attribute_config(ai_list));

            REQUIRE(device->is_attribute_polled(att));

            AND_GIVEN("an alarm event subscription")
            {
                EvCb callback;
                int evid;
                REQUIRE_NOTHROW(evid = device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                WHEN("we set the attribute value above max alarm")
                {
                    Tango::DeviceAttribute v;
                    v.set_name(att);
                    v << ATTR_MAX_ALARM + 1;
                    REQUIRE_NOTHROW(device->write_attribute(v));

                    THEN("an alarm event is generated")
                    {
                        REQUIRE(callback.test_last_event("alarm", ATTR_MAX_ALARM + 1, Tango::ATTR_ALARM));
                    }
                }

                WHEN("we set the attribute value back below max alarm")
                {
                    Tango::DeviceAttribute v;
                    v.set_name(att);
                    v << ATTR_INIT_VALUE;
                    REQUIRE_NOTHROW(device->write_attribute(v));

                    THEN("an alarm event is generated")
                    {
                        REQUIRE(callback.test_last_event("alarm", ATTR_INIT_VALUE, Tango::ATTR_VALID));
                    }
                }

                WHEN("we set the attribute value below min alarm")
                {
                    Tango::DeviceAttribute v;
                    v.set_name(att);
                    v << ATTR_MIN_ALARM - 1;
                    REQUIRE_NOTHROW(device->write_attribute(v));

                    THEN("an alarm event is generated")
                    {
                        REQUIRE(callback.test_last_event("alarm", ATTR_MIN_ALARM - 1, Tango::ATTR_ALARM));
                    }
                }

                WHEN("we set the attribute value back above min alarm")
                {
                    Tango::DeviceAttribute v;
                    v.set_name(att);
                    v << ATTR_INIT_VALUE;
                    REQUIRE_NOTHROW(device->write_attribute(v));

                    THEN("an alarm event is generated")
                    {
                        REQUIRE(callback.test_last_event("alarm", ATTR_INIT_VALUE, Tango::ATTR_VALID));
                    }
                }
            }
        }
    }
}

SCENARIO("Manual quality change triggers ALARM_EVENT")
{
    int idlver = GENERATE(range(6, 7));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute name")
        {
            std::string att{"attr_test"};

            REQUIRE(device->is_attribute_polled(att));

            AND_GIVEN("an alarm event subscription")
            {
                EvCb callback;
                int evid;
                REQUIRE_NOTHROW(evid = device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                WHEN("we set the attribute quality to ATTR_ALARM")
                {
                    REQUIRE_NOTHROW(device->command_inout("set_alarm"));

                    THEN("an alarm event is generated")
                    {
                        REQUIRE(callback.test_last_event("alarm", ATTR_INIT_VALUE, Tango::ATTR_ALARM));
                    }
                }

                WHEN("we set the attribute quality back to ATTR_VALID")
                {
                    REQUIRE_NOTHROW(device->command_inout("set_valid"));

                    THEN("an alarm event is generated")
                    {
                        REQUIRE(callback.test_last_event("alarm", ATTR_INIT_VALUE, Tango::ATTR_VALID));
                    }
                }
            }
        }
    }
}

SCENARIO("Alarm events can be pushed from code manually")
{
    int idlver = GENERATE(range(6, 7));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute which pushes events from code")
        {
            std::string att{"attr_push"};

            AND_GIVEN("an alarm event subscription")
            {
                EvCb callback;
                int evid;
                REQUIRE_NOTHROW(evid = device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                WHEN("we push an alarm event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                    THEN("an alarm event is generated")
                    {
                        REQUIRE(callback.test_last_event("alarm", ATTR_PUSH_VALUE, Tango::ATTR_VALID));
                    }
                }
            }
        }
    }
}
