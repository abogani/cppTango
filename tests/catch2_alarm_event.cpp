#include <tango/tango.h>

#include <memory>

#include "utils/utils.h"

static constexpr double ATTR_VALID_VALUE = 0.0;
static constexpr double ATTR_INIT_VALUE = ATTR_VALID_VALUE;
static constexpr double ATTR_MIN_WARNING = -1.0;
static constexpr double ATTR_MAX_WARNING = 1.0;
static constexpr double ATTR_MIN_ALARM = -5.0;
static constexpr double ATTR_MAX_ALARM = 5.0;
static constexpr double ATTR_PUSH_ALARM_VALUE = 10.0;

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

    void set_warning()
    {
        attr_quality = Tango::ATTR_WARNING;
    }

    void set_valid()
    {
        attr_quality = Tango::ATTR_VALID;
    }

    void push_alarm()
    {
        Tango::DevDouble v{ATTR_PUSH_ALARM_VALUE};
        this->push_alarm_event("attr_push", &v);
    }

    void push_change()
    {
        Tango::DevDouble v{ATTR_PUSH_ALARM_VALUE};
        this->push_change_event("attr_change", &v);
        this->push_change_event("attr_change_alarm", &v);
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
        Tango::UserDefaultAttrProp props;
        props.set_min_warning(std::to_string(ATTR_MIN_WARNING).c_str());
        props.set_max_warning(std::to_string(ATTR_MAX_WARNING).c_str());
        props.set_min_alarm(std::to_string(ATTR_MIN_ALARM).c_str());
        props.set_max_alarm(std::to_string(ATTR_MAX_ALARM).c_str());

        auto attr_test = new TangoTest::AutoAttr<&AlarmEventDev::read_attribute, &AlarmEventDev::write_attribute>(
            "attr_test", Tango::DEV_DOUBLE);
        attr_test->set_polling_period(100);
        attr_test->set_default_properties(props);
        attrs.push_back(attr_test);

        // attribute which pushes alarm events from code without checking criteria
        auto attr_push = new TangoTest::AutoAttr<&AlarmEventDev::read_attribute, &AlarmEventDev::write_attribute>(
            "attr_push", Tango::DEV_DOUBLE);
        attr_push->set_alarm_event(true, false);
        attrs.push_back(attr_push);
        // attribute which pushes change events from code
        auto attr_change = new TangoTest::AutoAttr<&AlarmEventDev::read_attribute, &AlarmEventDev::write_attribute>(
            "attr_change", Tango::DEV_DOUBLE);
        props.set_event_abs_change("0.1");
        attr_change->set_default_properties(props);
        attr_change->set_change_event(true, true);
        attrs.push_back(attr_change);

        // attribute which pushes change and alarm events from code
        auto attr_change_alarm =
            new TangoTest::AutoAttr<&AlarmEventDev::read_attribute, &AlarmEventDev::write_attribute>(
                "attr_change_alarm", Tango::DEV_DOUBLE);
        attr_change_alarm->set_default_properties(props);
        attr_change_alarm->set_change_event(true, true);
        attr_change_alarm->set_alarm_event(true, true);
        attrs.push_back(attr_change_alarm);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::set_warning>("set_warning"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::set_alarm>("set_alarm"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::set_valid>("set_valid"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::push_alarm>("push_alarm"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::push_change>("push_change"));
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

    bool test_event_received()
    {
        if(event_received)
        {
            event_received = false;
            return true;
        }
        return false;
    }

    bool test_last_event(std::string expected_event_type,
                         Tango::DevDouble expected_value,
                         Tango::AttrQuality expected_quality)
    {
        std::cout << "test: event_received=" << event_received << "; last_event_type=" << last_event_type
                  << "; last_event_value=" << last_event_value << "; last_event_quality=" << last_event_quality << "\n";
        if(test_event_received())
        {
            return ((last_event_type == expected_event_type) && (last_event_value == expected_value) &&
                    (last_event_quality == expected_quality));
        }
        return false;
    }

    void clear_event_flag()
    {
        event_received = false;
    }

  private:
    bool event_received;
    std::string last_event_type;
    Tango::DevDouble last_event_value;
    Tango::AttrQuality last_event_quality;
};

const char *attr_quality_name(Tango::AttrQuality qual)
{
    switch(qual)
    {
    case Tango::ATTR_VALID:
        return "ATTR_VALID";
    case Tango::ATTR_INVALID:
        return "ATTR_INVALID";
    case Tango::ATTR_ALARM:
        return "ATTR_ALARM";
    case Tango::ATTR_CHANGING:
        return "ATTR_CHANGING";
    case Tango::ATTR_WARNING:
        return "ATTR_WARNING";
    };

    return "UNKNOWN";
}

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

        struct NamedValue
        {
            const char *name;
            Tango::DevDouble value;
        };

        NamedValue valid{"VALID", ATTR_VALID_VALUE};
        NamedValue valid2{"different VALID", ATTR_VALID_VALUE + 0.5};
        NamedValue warning{"max WARNING", ATTR_MAX_WARNING + 1};
        NamedValue warning2{"different max WARNING", ATTR_MAX_WARNING + 2};
        NamedValue warning_min{"min WARNING", ATTR_MIN_WARNING - 1};
        NamedValue alarm{"max ALARM", ATTR_MAX_ALARM + 1};
        NamedValue alarm2{"different max ALARM", ATTR_MAX_ALARM + 2};
        NamedValue alarm_min{"min ALARM", ATTR_MIN_ALARM - 1};

        struct TestData
        {
            NamedValue initial;
            NamedValue final;
            std::optional<Tango::AttrQuality> event_quality;
        };

        auto data = GENERATE_REF(TestData{valid, warning, Tango::ATTR_WARNING},
                                 TestData{valid, alarm, Tango::ATTR_ALARM},
                                 TestData{valid, warning_min, Tango::ATTR_WARNING},
                                 TestData{valid, alarm_min, Tango::ATTR_ALARM},
                                 TestData{warning, alarm, Tango::ATTR_ALARM},
                                 TestData{warning, valid, Tango::ATTR_VALID},
                                 TestData{warning_min, valid, Tango::ATTR_VALID},
                                 TestData{warning_min, alarm_min, Tango::ATTR_ALARM},
                                 TestData{alarm_min, warning_min, Tango::ATTR_WARNING},
                                 TestData{alarm, warning, Tango::ATTR_WARNING},
                                 TestData{alarm, valid, Tango::ATTR_VALID},
                                 TestData{valid, valid2, std::nullopt},
                                 TestData{warning, warning2, std::nullopt},
                                 TestData{alarm, alarm2, std::nullopt});

        AND_GIVEN("a polled attribute with a " << data.initial.name << " value")
        {
            std::string att{"attr_test"};

            REQUIRE(device->is_attribute_polled(att));

            Tango::DeviceAttribute v;
            v.set_name(att);
            v << data.initial.value;
            REQUIRE_NOTHROW(device->write_attribute(v));

            AND_GIVEN("an alarm event subscription to that attribute")
            {
                EvCb callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                WHEN("we set the attribute to a " << data.final.value << " value")
                {
                    Tango::DeviceAttribute v;
                    v.set_name(att);
                    v << data.final.value;
                    callback.clear_event_flag();
                    REQUIRE_NOTHROW(device->write_attribute(v));

                    if(data.event_quality.has_value())
                    {
                        THEN("an alarm event is generated with " << attr_quality_name(*data.event_quality))
                        {
                            REQUIRE(callback.test_last_event("alarm", data.final.value, *data.event_quality));
                        }
                    }
                    else
                    {
                        THEN("no event is generated")
                        {
                            REQUIRE(!callback.test_event_received());
                        }
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

        struct TestData
        {
            const char *initial_cmd;
            const char *new_cmd;
            Tango::AttrQuality event_quality;
        };

        auto data = GENERATE(TestData{"set_valid", "set_warning", Tango::ATTR_WARNING},
                             TestData{"set_valid", "set_alarm", Tango::ATTR_ALARM},
                             TestData{"set_warning", "set_valid", Tango::ATTR_VALID},
                             TestData{"set_warning", "set_alarm", Tango::ATTR_ALARM},
                             TestData{"set_alarm", "set_valid", Tango::ATTR_VALID},
                             TestData{"set_alarm", "set_warning", Tango::ATTR_WARNING});

        // Skip the "set_"
        std::string initial_name = data.initial_cmd + 4;
        std::transform(initial_name.begin(), initial_name.end(), initial_name.begin(), ::toupper);
        AND_GIVEN("a polled attribute with " << initial_name << " quality")
        {
            std::string att{"attr_test"};

            REQUIRE(device->is_attribute_polled(att));

            REQUIRE_NOTHROW(device->command_inout(data.initial_cmd));

            AND_GIVEN("an alarm event subscription to that attribute")
            {
                EvCb callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                std::string new_name = data.new_cmd + 4;
                std::transform(new_name.begin(), new_name.end(), new_name.begin(), ::toupper);
                WHEN("we set the attribute quality to " << new_name)
                {
                    REQUIRE_NOTHROW(device->command_inout(data.new_cmd));

                    THEN("an alarm event is generated with " << attr_quality_name(data.event_quality))
                    {
                        REQUIRE(callback.test_last_event("alarm", ATTR_INIT_VALUE, data.event_quality));
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

        AND_GIVEN("an attribute which pushes events from code without checking criteria")
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
                        REQUIRE(callback.test_last_event("alarm", ATTR_PUSH_ALARM_VALUE, Tango::ATTR_VALID));
                    }
                }
            }
        }
    }
}

SCENARIO("Alarm events are pushed together with manual change events")
{
    int idlver = GENERATE(range(6, 7));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute which pushes change events from code")
        {
            std::string att{"attr_change"};

            EvCb change_cb, alarm_cb;
            int change_evid, alarm_evid;

            WHEN("we subscribe to change and alarm events (no polling on attribute)")
            {
                THEN("the subscription succeeds")
                {
                    REQUIRE_NOTHROW(change_evid = device->subscribe_event(att, Tango::CHANGE_EVENT, &change_cb));
                    REQUIRE_NOTHROW(alarm_evid = device->subscribe_event(att, Tango::ALARM_EVENT, &alarm_cb));

                    WHEN("we push a change event from code")
                    {
                        REQUIRE_NOTHROW(device->command_inout("push_change"));

                        THEN("a change and alarm events are generated")
                        {
                            REQUIRE(change_cb.test_last_event("change", ATTR_PUSH_ALARM_VALUE, Tango::ATTR_ALARM));
                            REQUIRE(alarm_cb.test_last_event("alarm", ATTR_PUSH_ALARM_VALUE, Tango::ATTR_ALARM));
                        }
                    }
                }
            }
        }

        AND_GIVEN("an attribute which pushes change and alarm events from code")
        {
            std::string att{"attr_change_alarm"};

            EvCb change_cb, alarm_cb;
            int change_evid, alarm_evid;

            WHEN("we subscribe to change and alarm events")
            {
                THEN("the subscription succeeds")
                {
                    REQUIRE_NOTHROW(change_evid = device->subscribe_event(att, Tango::CHANGE_EVENT, &change_cb));
                    REQUIRE_NOTHROW(alarm_evid = device->subscribe_event(att, Tango::ALARM_EVENT, &alarm_cb));

                    WHEN("we push a change event from code")
                    {
                        // clear event received on subscription
                        alarm_cb.clear_event_flag();
                        REQUIRE_NOTHROW(device->command_inout("push_change"));

                        THEN("a change event is generated but alarm event is not")
                        {
                            REQUIRE(change_cb.test_last_event("change", ATTR_PUSH_ALARM_VALUE, Tango::ATTR_ALARM));
                            REQUIRE(!alarm_cb.test_event_received());
                        }
                    }
                }
            }
        }
    }
}
