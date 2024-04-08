#include <catch2/matchers/catch_matchers.hpp>
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
        this->push_alarm_event("attr_test", &v);
        this->push_alarm_event("attr_push", &v);
    }

    void push_change()
    {
        Tango::DevDouble v{ATTR_PUSH_ALARM_VALUE};
        this->push_change_event("attr_test", &v);
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

        auto attr_no_polling = new TangoTest::AutoAttr<&AlarmEventDev::read_attribute, &AlarmEventDev::write_attribute>(
            "attr_no_polling", Tango::DEV_DOUBLE);
        attr_no_polling->set_default_properties(props);
        attrs.push_back(attr_no_polling);

        auto attr_test = new TangoTest::AutoAttr<&AlarmEventDev::read_attribute, &AlarmEventDev::write_attribute>(
            "attr_test", Tango::DEV_DOUBLE);
        attr_test->set_polling_period(100);
        attr_test->set_default_properties(props);
        attrs.push_back(attr_test);

        // attribute which pushes alarm events from code without checking criteria
        auto attr_push = new TangoTest::AutoAttr<&AlarmEventDev::read_attribute, &AlarmEventDev::write_attribute>(
            "attr_push", Tango::DEV_DOUBLE);
        attr_push->set_default_properties(props);
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
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                // discard the two initial events we get when we subscribe
                auto maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());
                maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());

                WHEN("we set the attribute to a " << data.final.value << " value")
                {
                    Tango::DeviceAttribute v;
                    v.set_name(att);

                    v << data.final.value;
                    REQUIRE_NOTHROW(device->write_attribute(v));

                    if(data.event_quality.has_value())
                    {
                        THEN("an alarm event is generated with " << attr_quality_name(*data.event_quality))
                        {
                            auto maybe_event = callback.pop_next_event();

                            REQUIRE(maybe_event.has_value());
                            REQUIRE(!maybe_event->err);
                            REQUIRE(maybe_event->event == "alarm");

                            REQUIRE(maybe_event->attr_value != nullptr);
                            REQUIRE(maybe_event->attr_value->get_quality() == *data.event_quality);

                            std::vector<Tango::DevDouble> expected{data.final.value, data.final.value};
                            REQUIRE_THAT(*maybe_event->attr_value, TangoTest::AnyLikeContains(expected));
                        }
                    }
                    else
                    {
                        THEN("no event is generated")
                        {
                            auto maybe_event = callback.pop_next_event(std::chrono::milliseconds{200});
                            REQUIRE(!maybe_event.has_value());
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
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                // discard the two initial events we get when we subscribe
                auto maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());
                maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());

                std::string new_name = data.new_cmd + 4;
                std::transform(new_name.begin(), new_name.end(), new_name.begin(), ::toupper);
                WHEN("we set the attribute quality to " << new_name)
                {
                    REQUIRE_NOTHROW(device->command_inout(data.new_cmd));

                    THEN("an alarm event is generated with " << attr_quality_name(data.event_quality))
                    {
                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event.has_value());
                        REQUIRE(!maybe_event->err);
                        REQUIRE(maybe_event->event == "alarm");

                        REQUIRE(maybe_event->attr_value != nullptr);
                        REQUIRE(maybe_event->attr_value->get_quality() == data.event_quality);
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
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                // discard the two initial events we get when we subscribe
                auto maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());

                WHEN("we push an alarm event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                    THEN("an alarm event is generated")
                    {
                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event.has_value());
                        REQUIRE(!maybe_event->err);
                        REQUIRE(maybe_event->event == "alarm");

                        REQUIRE(maybe_event->attr_value != nullptr);
                        REQUIRE(maybe_event->attr_value->get_quality() == Tango::ATTR_ALARM);
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

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe to alarm events (no polling on attribute)")
            {
                THEN("the subscription succeeds")
                {
                    REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                    // discard the initial event we get when we subscribe
                    auto maybe_initial_event = callback.pop_next_event();
                    REQUIRE(maybe_initial_event.has_value());

                    WHEN("we push a change event from code")
                    {
                        REQUIRE_NOTHROW(device->command_inout("push_change"));

                        THEN("a change and alarm events are generated")
                        {
                            auto maybe_event = callback.pop_next_event();

                            REQUIRE(maybe_event.has_value());
                            REQUIRE(!maybe_event->err);
                            REQUIRE(maybe_event->event == "alarm");

                            REQUIRE(maybe_event->attr_value != nullptr);
                            REQUIRE(maybe_event->attr_value->get_quality() == Tango::ATTR_ALARM);
                        }
                    }
                }
            }
        }

        AND_GIVEN("an attribute which pushes change and alarm events from code")
        {
            std::string att{"attr_change_alarm"};

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe to change and alarm events")
            {
                THEN("the subscription succeeds")
                {
                    REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                    // discard the initial event we get when we subscribe
                    auto maybe_initial_event = callback.pop_next_event();
                    REQUIRE(maybe_initial_event.has_value());

                    WHEN("we push a change event from code")
                    {
                        REQUIRE_NOTHROW(device->command_inout("push_change"));

                        THEN("no alarm event is generated")
                        {
                            auto maybe_event = callback.pop_next_event(std::chrono::milliseconds{200});
                            REQUIRE(!maybe_event.has_value());
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Subscribing to alarm events for an attribute with no polling fails")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();
        AND_GIVEN("an attribute with no polling")
        {
            std::string att{"attr_no_polling"};

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe to alarm events")
            {
                THEN("the subscription fails")
                {
                    REQUIRE_THROWS_MATCHES(device->subscribe_event(att, Tango::ALARM_EVENT, &callback),
                                           Tango::DevFailed,
                                           TangoTest::DevFailedReasonEquals(Tango::API_AttributePollingNotStarted));
                }
            }
        }
    }
}

SCENARIO("Auto alarm on change events can be disabled")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{
            "alarm_event", "AlarmEventDev", idlver, "FREE/CtrlSystem->AutoAlarmOnChangeEvent: false\n"};

        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute which pushes change events from code")
        {
            std::string att{"attr_change"};

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe to alarm_events")
            {
                THEN("the subscription fails")
                {
                    REQUIRE_THROWS_MATCHES(device->subscribe_event(att, Tango::ALARM_EVENT, &callback),
                                           Tango::DevFailed,
                                           TangoTest::DevFailedReasonEquals(Tango::API_AttributePollingNotStarted));
                }
            }
        }
    }
}

SCENARIO("Subscribing to alarm events from a missing attribute fails")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();
        AND_GIVEN("a missing attribute")
        {
            std::string att{"attr_missing"};

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe to alarm events")
            {
                THEN("the subscription fails")
                {
                    REQUIRE_THROWS_MATCHES(device->subscribe_event(att, Tango::ALARM_EVENT, &callback),
                                           Tango::DevFailed,
                                           TangoTest::DevFailedReasonEquals(Tango::API_AttrNotFound));
                }
            }
        }
    }
}

SCENARIO("Pushing events for a polled attribute works")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute")
        {
            std::string att{"attr_test"};

            AND_GIVEN("an alarm event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                // discard the two initial events we get when we subscribe
                auto maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());
                maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());

                WHEN("we push an alarm event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                    THEN("an alarm event is generated")
                    {
                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event.has_value());
                        REQUIRE(!maybe_event->err);
                        REQUIRE(maybe_event->event == "alarm");

                        REQUIRE(maybe_event->attr_value != nullptr);
                        REQUIRE(maybe_event->attr_value->get_quality() == Tango::ATTR_ALARM);
                    }
                }

                WHEN("we push a change event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_change"));

                    THEN("an alarm event is generated")
                    {
                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event.has_value());
                        REQUIRE(!maybe_event->err);
                        REQUIRE(maybe_event->event == "alarm");

                        REQUIRE(maybe_event->attr_value != nullptr);
                        REQUIRE(maybe_event->attr_value->get_quality() == Tango::ATTR_ALARM);
                    }
                }
            }
        }
    }
}

SCENARIO("Pushing alarm events from push_change_event on polled attributes can be disabled")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{
            "alarm_event", "AlarmEventDev", idlver, "FREE/CtrlSystem->AutoAlarmOnChangeEvent: false\n"};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute")
        {
            std::string att{"attr_test"};

            AND_GIVEN("an alarm event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback));

                // discard the two initial events we get when we subscribe
                auto maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());
                maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());

                WHEN("we push an alarm event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                    THEN("an alarm event is generated")
                    {
                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event.has_value());
                        REQUIRE(!maybe_event->err);
                        REQUIRE(maybe_event->event == "alarm");

                        REQUIRE(maybe_event->attr_value != nullptr);
                        REQUIRE(maybe_event->attr_value->get_quality() == Tango::ATTR_ALARM);
                    }
                }

                WHEN("we push a change event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_change"));

                    THEN("no alarm event is generated")
                    {
                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(!maybe_event.has_value());
                    }
                }
            }
        }
    }
}

template <class Base>
class SpectrumAlarmEvent : public Base
{
  public:
    using Base::Base;

    ~SpectrumAlarmEvent() override { }

    void init_device() override
    {
        attr_value = {ATTR_INIT_VALUE, ATTR_INIT_VALUE, ATTR_INIT_VALUE};
    }

    void push_change()
    {
        std::vector<Tango::DevDouble> v{ATTR_INIT_VALUE, ATTR_PUSH_ALARM_VALUE, ATTR_INIT_VALUE};
        this->push_change_event("attr_test", v.data(), v.size());
    }

    void read_attribute(Tango::Attribute &att)
    {
        att.set_value(attr_value.data(), attr_value.size());
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        Tango::UserDefaultAttrProp props;
        props.set_min_warning(std::to_string(ATTR_MIN_WARNING).c_str());
        props.set_max_warning(std::to_string(ATTR_MAX_WARNING).c_str());
        props.set_min_alarm(std::to_string(ATTR_MIN_ALARM).c_str());
        props.set_max_alarm(std::to_string(ATTR_MAX_ALARM).c_str());
        props.set_event_abs_change("0.1");

        auto attr_test =
            new TangoTest::AutoSpectrumAttr<&SpectrumAlarmEvent::read_attribute>("attr_test", Tango::DEV_DOUBLE, 3);
        attr_test->set_default_properties(props);
        attr_test->set_change_event(true, true);
        attrs.push_back(attr_test);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&SpectrumAlarmEvent::push_change>("push_change"));
    }

  private:
    std::vector<Tango::DevDouble> attr_value;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(SpectrumAlarmEvent, 6)

SCENARIO("Alarm events are generated for spectrum attributes on push_change_event")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "SpectrumAlarmEvent", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a spetcrum attribute")
        {
            std::string att{"attr_test"};

            AND_GIVEN("an alarm event and change event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback_alarm;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback_alarm));

                TangoTest::CallbackMock<Tango::EventData> callback_change;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::CHANGE_EVENT, &callback_change));

                // discard the initial events we get when we subscribe
                auto maybe_initial_event = callback_alarm.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());
                maybe_initial_event = callback_change.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());

                WHEN("we push a change event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_change"));

                    THEN("alarm and change events are generated")
                    {
                        {
                            auto maybe_event = callback_alarm.pop_next_event();

                            REQUIRE(maybe_event.has_value());
                            REQUIRE(!maybe_event->err);
                            REQUIRE(maybe_event->event == "alarm");

                            REQUIRE(maybe_event->attr_value != nullptr);
                            REQUIRE(maybe_event->attr_value->get_quality() == Tango::ATTR_ALARM);
                        }

                        {
                            auto maybe_event = callback_change.pop_next_event();

                            REQUIRE(maybe_event.has_value());
                            REQUIRE(!maybe_event->err);
                            REQUIRE(maybe_event->event == "change");

                            REQUIRE(maybe_event->attr_value != nullptr);
                            REQUIRE(maybe_event->attr_value->get_quality() == Tango::ATTR_ALARM);
                        }
                    }
                }
            }

            AND_GIVEN("only a change event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback_change;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::CHANGE_EVENT, &callback_change));

                // discard the initial event we get when we subscribe
                auto maybe_initial_event = callback_change.pop_next_event();
                REQUIRE(maybe_initial_event.has_value());

                WHEN("we push a change event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_change"));

                    THEN("a change event is generated")
                    {
                        auto maybe_event = callback_change.pop_next_event();

                        REQUIRE(maybe_event.has_value());
                        REQUIRE(!maybe_event->err);
                        REQUIRE(maybe_event->event == "change");

                        REQUIRE(maybe_event->attr_value != nullptr);
                        REQUIRE(maybe_event->attr_value->get_quality() == Tango::ATTR_ALARM);
                    }
                }
            }
        }
    }
}
