#include "catch2_common.h"

#include <catch2/matchers/catch_matchers.hpp>

static constexpr double ATTR_VALID_VALUE = 0.0;
static constexpr double ATTR_INIT_VALUE = ATTR_VALID_VALUE;
static constexpr double ATTR_MIN_WARNING = -1.0;
static constexpr double ATTR_MAX_WARNING = 1.0;
static constexpr double ATTR_MIN_ALARM = -5.0;
static constexpr double ATTR_MAX_ALARM = 5.0;
static constexpr double ATTR_PUSH_ALARM_VALUE = 10.0;

constexpr static const char *k_test_reason = "Test_Reason";
constexpr static const char *k_alt_test_reason = "Test_AltReason";
constexpr static const char *k_a_helpful_desc = "A helpful description";
constexpr static const int k_polling_period = TANGO_TEST_CATCH2_DEFAULT_POLL_PERIOD;

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
        throw_next_read = false;
        except_next_push = false;
        alt_except_next_push = false;
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
        if(except_next_push)
        {
            except_next_push = false;
            try
            {
                TANGO_THROW_EXCEPTION(k_test_reason, k_a_helpful_desc);
            }
            catch(Tango::DevFailed &e)
            {
                TANGO_LOG_DEBUG << "Pushing error ALARM_EVENT to \"attr_push\"";
                this->push_alarm_event("attr_push", &e);

                TANGO_LOG_DEBUG << "Pushing error ALARM_EVENT to \"attr_change_alarm\"";
                this->push_alarm_event("attr_change_alarm", &e);
                return;
            }
        }

        if(alt_except_next_push)
        {
            alt_except_next_push = false;
            try
            {
                TANGO_THROW_EXCEPTION(k_alt_test_reason, k_a_helpful_desc);
            }
            catch(Tango::DevFailed &e)
            {
                TANGO_LOG_DEBUG << "Pushing alternative error ALARM_EVENT to \"attr_push\"";
                this->push_alarm_event("attr_push", &e);

                TANGO_LOG_DEBUG << "Pushing alternative error ALARM_EVENT to \"attr_change_alarm\"";
                this->push_alarm_event("attr_change_alarm", &e);
                return;
            }
        }

        Tango::DevDouble v{ATTR_PUSH_ALARM_VALUE};
        TANGO_LOG_DEBUG << "Pushing ALARM_EVENT with value " << v << " to \"attr_test\"";
        this->push_alarm_event("attr_test", &v);

        TANGO_LOG_DEBUG << "Pushing ALARM_EVENT with value " << v << " to \"attr_push\"";
        this->push_alarm_event("attr_push", &v);

        TANGO_LOG_DEBUG << "Pushing ALARM_EVENT with value " << v << " to \"attr_change_alarm\"";
        this->push_alarm_event("attr_change_alarm", &v);
    }

    void push_change()
    {
        if(except_next_push)
        {
            except_next_push = false;
            try
            {
                TANGO_THROW_EXCEPTION(k_test_reason, k_a_helpful_desc);
            }
            catch(Tango::DevFailed &e)
            {
                TANGO_LOG_DEBUG << "Pushing error CHANGE_EVENT to \"attr_change\"";
                this->push_change_event("attr_change", &e);

                TANGO_LOG_DEBUG << "Pushing alternative error CHANGE_EVENT to \"attr_change_alarm\"";
                this->push_change_event("attr_change_alarm", &e);
                return;
            }
        }

        if(alt_except_next_push)
        {
            alt_except_next_push = false;
            try
            {
                TANGO_THROW_EXCEPTION(k_alt_test_reason, k_a_helpful_desc);
            }
            catch(Tango::DevFailed &e)
            {
                TANGO_LOG_DEBUG << "Pushing altnerative error CHANGE_EVENT to \"attr_change\"";
                this->push_change_event("attr_change", &e);

                TANGO_LOG_DEBUG << "Pushing alternative error CHANGE_EVENT to \"attr_change_alarm\"";
                this->push_change_event("attr_change_alarm", &e);
                return;
            }
        }
        Tango::DevDouble v{ATTR_PUSH_ALARM_VALUE};
        TANGO_LOG_DEBUG << "Pushing CHANGE_EVENT with value " << v << " to \"attr_test\"";
        this->push_change_event("attr_test", &v);

        TANGO_LOG_DEBUG << "Pushing CHANGE_EVENT with value " << v << " to \"attr_change\"";
        this->push_change_event("attr_change", &v);

        TANGO_LOG_DEBUG << "Pushing CHANGE_EVENT with value " << v << " to \"attr_change_alarm\"";
        this->push_change_event("attr_change_alarm", &v);
    }

    void read_attribute(Tango::Attribute &att)
    {
        if(throw_next_read)
        {
            throw_next_read = false;
            TANGO_LOG_DEBUG << "Throwing from read_attribute";
            TANGO_THROW_EXCEPTION(k_test_reason, k_a_helpful_desc);
        }

        att.set_value_date_quality(&attr_value, std::chrono::system_clock::now(), attr_quality);
        TANGO_LOG_DEBUG << "Read value " << attr_value << " and quality " << attr_quality;
    }

    void write_attribute(Tango::WAttribute &att)
    {
        att.get_write_value(attr_value);
        TANGO_LOG_DEBUG << "Written value " << attr_value;
    }

    void throw_on_next_read()
    {
        throw_next_read = true;
    }

    void push_except_next()
    {
        except_next_push = true;
    }

    void push_alt_except_next()
    {
        alt_except_next_push = true;
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
        attr_test->set_polling_period(k_polling_period);
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
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::throw_on_next_read>("throw_on_next_read"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::push_except_next>("push_except_next"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmEventDev::push_alt_except_next>("push_alt_except_next"));
    }

  private:
    bool throw_next_read;
    bool except_next_push;
    bool alt_except_next_push;

    Tango::DevDouble attr_value;
    Tango::AttrQuality attr_quality;
};

// Alarm event is supported from IDL6 onwards
TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AlarmEventDev, 6)

SCENARIO("Attribute alarm range triggers ALARM_EVENT")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

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

            TANGO_LOG_DEBUG << "attribute name = \"" << att << "\"";

            REQUIRE(device->is_attribute_polled(att));

            Tango::DeviceAttribute v;
            v.set_name(att);
            v << data.initial.value;
            REQUIRE_NOTHROW(device->write_attribute(v));

            AND_GIVEN("an alarm event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback};

                require_initial_events(callback);

                WHEN("we set the attribute to a " << data.final.name << " value")
                {
                    using namespace Catch::Matchers;
                    using namespace TangoTest::Matchers;

                    Tango::DeviceAttribute v;
                    v.set_name(att);

                    v << data.final.value;
                    REQUIRE_NOTHROW(device->write_attribute(v));

                    if(data.event_quality.has_value())
                    {
                        THEN("an alarm event is generated with " << *data.event_quality)
                        {
                            using namespace TangoTest::Matchers;

                            auto maybe_event = callback.pop_next_event();

                            REQUIRE(maybe_event != std::nullopt);
                            REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                            REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(*data.event_quality)));

                            std::vector<Tango::DevDouble> expected{data.final.value, data.final.value};
                            REQUIRE_THAT(maybe_event, EventValueMatches(AnyLikeContains(expected)));
                        }
                    }
                    else
                    {
                        THEN("no event is generated")
                        {
                            auto maybe_event = callback.pop_next_event(std::chrono::milliseconds{200});
                            REQUIRE(maybe_event == std::nullopt);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Alarm events are sent on a read attribute exception")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a subscription to a polled attribute")
        {
            std::string att{"attr_test"};

            TangoTest::CallbackMock<Tango::EventData> callback;
            TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback};

            require_initial_events(callback);

            WHEN("the attribute read callback throws an exception once")
            {
                REQUIRE_NOTHROW(device->command_inout("throw_on_next_read"));

                THEN("we recieve an error alarm event")
                {
                    using namespace Catch::Matchers;
                    using namespace TangoTest::Matchers;

                    auto maybe_ex_event = callback.pop_next_event();

                    REQUIRE(maybe_ex_event != std::nullopt);
                    REQUIRE_THAT(maybe_ex_event, EventType(Tango::ALARM_EVENT));
                    REQUIRE_THAT(maybe_ex_event,
                                 EventErrorMatches(
                                     AllMatch(Reason(k_test_reason) && DescriptionMatches(Equals(k_a_helpful_desc)))));

                    AND_THEN("we recieve a normal alarm event")
                    {
                        auto maybe_good_event = callback.pop_next_event();

                        REQUIRE(maybe_good_event != std::nullopt);
                        REQUIRE_THAT(maybe_good_event, EventType(Tango::ALARM_EVENT));
                        REQUIRE_THAT(maybe_good_event, EventValueMatches(AttrQuality(Tango::ATTR_VALID)));

                        maybe_good_event = callback.pop_next_event(std::chrono::milliseconds{200});
                        REQUIRE(maybe_good_event == std::nullopt);
                    }
                }
            }
        }
    }
}

SCENARIO("Manual quality change triggers ALARM_EVENT")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

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
                TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback};

                require_initial_events(callback);

                std::string new_name = data.new_cmd + 4;
                std::transform(new_name.begin(), new_name.end(), new_name.begin(), ::toupper);
                WHEN("we set the attribute quality to " << new_name)
                {
                    REQUIRE_NOTHROW(device->command_inout(data.new_cmd));

                    THEN("an alarm event is generated with " << data.event_quality)
                    {
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                        REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(data.event_quality)));
                    }
                }
            }
        }
    }
}

SCENARIO("Alarm events can be pushed from code manually")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        struct TestData
        {
            bool checks;
            const char *name;
        };

        auto data = GENERATE(TestData{false, "attr_push"}, TestData{true, "attr_change_alarm"});

        AND_GIVEN("an attribute which pushes events from code " << (data.checks ? "with" : "without")
                                                                << " checking criteria")
        {
            std::string att{data.name};

            AND_GIVEN("an alarm event subscription")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback};

                require_event(callback);

                WHEN("we push an alarm event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                    THEN("an alarm event is generated")
                    {
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                        REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));

                        AND_WHEN("we push another alarm event from code")
                        {
                            REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                            if(!data.checks)
                            {
                                THEN("another alarm event is generated")
                                {
                                    auto maybe_event = callback.pop_next_event();

                                    REQUIRE(maybe_event != std::nullopt);
                                    REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                                    REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                                }
                            }
                            else
                            {
                                THEN("no event is generated")
                                {
                                    auto maybe_event = callback.pop_next_event();

                                    REQUIRE(maybe_event == std::nullopt);
                                }
                            }
                        }
                    }
                }

                WHEN("we push an exception from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_except_next"));
                    REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                    THEN("an error alarm event is generated")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_ex_event = callback.pop_next_event();

                        REQUIRE(maybe_ex_event != std::nullopt);
                        REQUIRE_THAT(maybe_ex_event, EventType(Tango::ALARM_EVENT));
                        REQUIRE_THAT(maybe_ex_event,
                                     EventErrorMatches(AllMatch(Reason(k_test_reason) &&
                                                                DescriptionMatches(Equals(k_a_helpful_desc)))));

                        AND_WHEN("we push a normal event")
                        {
                            REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                            THEN("a normal alarm event is generated")
                            {
                                using namespace TangoTest::Matchers;

                                auto maybe_event = callback.pop_next_event();

                                REQUIRE(maybe_event != std::nullopt);
                                REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                                REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                            }
                        }

                        AND_WHEN("we push another exception with the same reason")
                        {
                            REQUIRE_NOTHROW(device->command_inout("push_except_next"));
                            REQUIRE_NOTHROW(device->command_inout("push_alarm"));
                            if(!data.checks)
                            {
                                THEN("an error alarm event is generated")
                                {
                                    using namespace Catch::Matchers;
                                    using namespace TangoTest::Matchers;

                                    auto maybe_event = callback.pop_next_event();

                                    REQUIRE(maybe_event != std::nullopt);
                                    REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                                    REQUIRE_THAT(
                                        maybe_event,
                                        EventErrorMatches(AllMatch(Reason(k_test_reason) &&
                                                                   DescriptionMatches(Equals(k_a_helpful_desc)))));
                                }
                            }
                            else
                            {
                                THEN("no event is generated")
                                {
                                    auto maybe_event = callback.pop_next_event();

                                    REQUIRE(maybe_event == std::nullopt);
                                }
                            }
                        }

                        AND_WHEN("we push another exception with a different reason")
                        {
                            REQUIRE_NOTHROW(device->command_inout("push_alt_except_next"));
                            REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                            THEN("an error alarm event is generated")
                            {
                                using namespace Catch::Matchers;
                                using namespace TangoTest::Matchers;

                                auto maybe_event = callback.pop_next_event();

                                REQUIRE(maybe_event != std::nullopt);
                                REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                                REQUIRE_THAT(maybe_event,
                                             EventErrorMatches(AllMatch(Reason(k_alt_test_reason) &&
                                                                        DescriptionMatches(Equals(k_a_helpful_desc)))));
                            }
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Alarm events are pushed together with manual change events")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute which pushes change events from code")
        {
            std::string att{"attr_change"};

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe to alarm events (no polling on attribute)")
            {
                THEN("the subscription succeeds")
                {
                    TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback};

                    require_event(callback);

                    WHEN("we push a change event from code")
                    {
                        REQUIRE_NOTHROW(device->command_inout("push_change"));

                        THEN("an alarm events are generated")
                        {
                            using namespace Catch::Matchers;
                            using namespace TangoTest::Matchers;

                            auto maybe_event = callback.pop_next_event();

                            REQUIRE(maybe_event != std::nullopt);
                            REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                            REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                        }
                    }

                    WHEN("we push an exception with push_change_event from code")
                    {
                        REQUIRE_NOTHROW(device->command_inout("push_except_next"));
                        REQUIRE_NOTHROW(device->command_inout("push_change"));

                        THEN("an error alarm event is generated")
                        {
                            using namespace Catch::Matchers;
                            using namespace TangoTest::Matchers;

                            auto maybe_ex_event = callback.pop_next_event();
                            REQUIRE(maybe_ex_event != std::nullopt);
                            REQUIRE_THAT(maybe_ex_event, EventType(Tango::ALARM_EVENT));
                            REQUIRE_THAT(maybe_ex_event,
                                         EventErrorMatches(AllMatch(Reason(k_test_reason) &&
                                                                    DescriptionMatches(Equals(k_a_helpful_desc)))));

                            AND_WHEN("we push a normal event")
                            {
                                REQUIRE_NOTHROW(device->command_inout("push_change"));

                                THEN("a normal alarm event is generated")
                                {
                                    using namespace Catch::Matchers;
                                    using namespace TangoTest::Matchers;

                                    auto maybe_event = callback.pop_next_event();

                                    REQUIRE(maybe_event != std::nullopt);
                                    REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                                    REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                                }
                            }

                            AND_WHEN("we push another exception with the same reason")
                            {
                                REQUIRE_NOTHROW(device->command_inout("push_except_next"));
                                REQUIRE_NOTHROW(device->command_inout("push_change"));
                                THEN("no event is generated")
                                {
                                    auto maybe_event = callback.pop_next_event();

                                    REQUIRE(maybe_event == std::nullopt);
                                }
                            }

                            AND_WHEN("we push another exception with a different reason")
                            {
                                REQUIRE_NOTHROW(device->command_inout("push_alt_except_next"));
                                REQUIRE_NOTHROW(device->command_inout("push_change"));

                                THEN("an error alarm event is generated")
                                {
                                    using namespace Catch::Matchers;
                                    using namespace TangoTest::Matchers;

                                    auto maybe_event = callback.pop_next_event();

                                    REQUIRE(maybe_event != std::nullopt);
                                    REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                                    REQUIRE_THAT(
                                        maybe_event,
                                        EventErrorMatches(AllMatch(Reason(k_alt_test_reason) &&
                                                                   DescriptionMatches(Equals(k_a_helpful_desc)))));
                                }
                            }
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
                    TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback};

                    require_event(callback);

                    WHEN("we push a change event from code")
                    {
                        REQUIRE_NOTHROW(device->command_inout("push_change"));

                        THEN("no alarm event is generated")
                        {
                            auto maybe_event = callback.pop_next_event(std::chrono::milliseconds{200});
                            REQUIRE(maybe_event == std::nullopt);
                        }
                    }

                    WHEN("we push an exception with push_change_event from code")
                    {
                        REQUIRE_NOTHROW(device->command_inout("push_except_next"));
                        REQUIRE_NOTHROW(device->command_inout("push_change"));
                        THEN("no alarm event is generated")
                        {
                            auto maybe_event = callback.pop_next_event(std::chrono::milliseconds{200});
                            REQUIRE(maybe_event == std::nullopt);
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
        TangoTest::Context ctx{"no_polling", "AttrPollingEvents", idlver};
        auto device = ctx.get_proxy();
        AND_GIVEN("an attribute with no polling")
        {
            std::string att{"attr_no_polling"};

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe with stateless=false to alarm events")
            {
                THEN("the subscription fails")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(device->subscribe_event(att, Tango::ALARM_EVENT, &callback, false),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_AttributePollingNotStarted)));
                }
            }

            WHEN("we subscribe with stateless=true to alarm events")
            {
                THEN("the subscription succeeds")
                {
                    REQUIRE_NOTHROW(device->subscribe_event(att, Tango::ALARM_EVENT, &callback, true));

                    AND_THEN("we receive an error event")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_initial_event = callback.pop_next_event();

                        REQUIRE(maybe_initial_event != std::nullopt);
                        REQUIRE_THAT(maybe_initial_event->errors,
                                     !IsEmpty() && AnyMatch(Reason(Tango::API_AttributePollingNotStarted)));
                    }
                }
            }
        }
    }
}

SCENARIO("Alarm events work with stateless=true")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute with a VALID value")
        {
            std::string att{"attr_test"};

            REQUIRE(device->is_attribute_polled(att));

            Tango::DeviceAttribute v;
            v.set_name(att);
            v << ATTR_INIT_VALUE;
            REQUIRE_NOTHROW(device->write_attribute(v));

            AND_GIVEN("a stateless alarm event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback, true};

                require_initial_events(callback);

                WHEN("we set the attribute to a max WARNING value")
                {
                    Tango::DeviceAttribute v;
                    v.set_name(att);
                    v << ATTR_MAX_WARNING + 1;
                    REQUIRE_NOTHROW(device->write_attribute(v));

                    THEN("an alarm event is generated with ATTR_WARNING")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                        REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_WARNING)));

                        std::vector<Tango::DevDouble> expected{ATTR_MAX_WARNING + 1, ATTR_MAX_WARNING + 1};
                        REQUIRE_THAT(maybe_event, EventValueMatches(AnyLikeContains(expected)));
                    }
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

        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute which pushes change events from code")
        {
            std::string att{"attr_change"};

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe to alarm_events")
            {
                THEN("the subscription fails")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(device->subscribe_event(att, Tango::ALARM_EVENT, &callback),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_AttributePollingNotStarted)));
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
        auto device = ctx.get_proxy();
        AND_GIVEN("a missing attribute")
        {
            std::string att{"attr_missing"};

            TangoTest::CallbackMock<Tango::EventData> callback;

            WHEN("we subscribe to alarm events")
            {
                THEN("the subscription fails")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(device->subscribe_event(att, Tango::ALARM_EVENT, &callback),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_AttrNotFound)));
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
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute")
        {
            std::string att{"attr_test"};

            AND_GIVEN("an alarm event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback};

                require_initial_events(callback);

                WHEN("we push an alarm event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                    THEN("an alarm event is generated")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                        REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                    }
                }

                WHEN("we push a change event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_change"));

                    THEN("an alarm event is generated")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                        REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                    }
                }
            }
        }
    }
}

SCENARIO("Alarm events subscription can be reconnected", "[slow]")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm_event", "AlarmEventDev", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute with a VALID value")
        {
            std::string att{"attr_test"};

            REQUIRE(device->is_attribute_polled(att));

            Tango::DeviceAttribute v;
            v.set_name(att);
            v << ATTR_INIT_VALUE;
            REQUIRE_NOTHROW(device->write_attribute(v));

            AND_GIVEN("an alarm event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback};

                require_initial_events(callback);

                WHEN("when we stop the server")
                {
                    ctx.stop_server();

                    THEN("a error event is generated")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event(std::chrono::seconds{20});

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventErrorMatches(AllMatch(Reason(Tango::API_EventTimeout))));

                        AND_WHEN("we restart the server")
                        {
                            ctx.restart_server();

                            THEN("an alarm event is generated after another error event")
                            {
                                using namespace Catch::Matchers;
                                using namespace TangoTest::Matchers;

                                auto maybe_event = callback.pop_next_event(std::chrono::seconds{20});

                                REQUIRE(maybe_event != std::nullopt);
                                REQUIRE_THAT(maybe_event, EventErrorMatches(AllMatch(Reason(Tango::API_EventTimeout))));

                                maybe_event = callback.pop_next_event(std::chrono::seconds{20});

                                REQUIRE(maybe_event != std::nullopt);
                                REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                            }
                        }
                    }
                }

                WHEN("when we polling the attribute")
                {
                    REQUIRE_NOTHROW(device->stop_poll_attribute(att));

                    THEN("a error event is generated")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event(std::chrono::seconds{20});

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventErrorMatches(AllMatch(Reason(Tango::API_PollObjNotFound))));

                        AND_WHEN("we reenable polling")
                        {
                            REQUIRE_NOTHROW(device->poll_attribute(att, k_polling_period));

                            THEN("an alarm event is generated")
                            {
                                using namespace Catch::Matchers;
                                using namespace TangoTest::Matchers;

                                maybe_event = callback.pop_next_event();

                                REQUIRE(maybe_event != std::nullopt);
                                REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                            }
                        }
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
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute")
        {
            std::string att{"attr_test"};

            AND_GIVEN("an alarm event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                TangoTest::Subscription sub{device, att, Tango::ALARM_EVENT, &callback};

                require_initial_events(callback);

                WHEN("we push an alarm event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_alarm"));

                    THEN("an alarm event is generated")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                        REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                    }
                }

                WHEN("we push a change event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_change"));

                    THEN("no alarm event is generated")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event == std::nullopt);
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
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a spetcrum attribute")
        {
            std::string att{"attr_test"};

            AND_GIVEN("an alarm event and change event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback_alarm;
                TangoTest::Subscription subscription_alarm{device, att, Tango::ALARM_EVENT, &callback_alarm};

                require_event(callback_alarm);

                TangoTest::CallbackMock<Tango::EventData> callback_change;
                TangoTest::Subscription subscription_change{device, att, Tango::CHANGE_EVENT, &callback_change};

                require_event(callback_change);

                WHEN("we push a change event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_change"));

                    THEN("alarm and change events are generated")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        {
                            auto maybe_event = callback_alarm.pop_next_event();

                            REQUIRE(maybe_event != std::nullopt);
                            REQUIRE_THAT(maybe_event, EventType(Tango::ALARM_EVENT));
                            REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                        }

                        {
                            auto maybe_event = callback_change.pop_next_event();

                            REQUIRE(maybe_event != std::nullopt);
                            REQUIRE_THAT(maybe_event, EventType(Tango::CHANGE_EVENT));
                            REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                        }
                    }
                }
            }

            AND_GIVEN("only a change event subscription to that attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback_change;
                TangoTest::Subscription subscription_change{device, att, Tango::CHANGE_EVENT, &callback_change};

                require_event(callback_change);

                WHEN("we push a change event from code")
                {
                    REQUIRE_NOTHROW(device->command_inout("push_change"));

                    THEN("a change event is generated")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback_change.pop_next_event();

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventType(Tango::CHANGE_EVENT));
                        REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));
                    }
                }
            }
        }
    }
}
