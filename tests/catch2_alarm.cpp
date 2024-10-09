#include "catch2_common.h"
#include "tango/server/except.h"

constexpr static const Tango::DevDouble k_alarm_level = 20;
constexpr static const Tango::DevDouble k_alarming_value = 99;

constexpr static const char *k_test_reason = "Test_Reason";
constexpr static const char *k_a_helpful_desc = "A helpful description";

template <class Base>
class AlarmDev : public Base
{
  public:
    using Base::Base;

    ~AlarmDev() override { }

    void init_device() override { }

    bool has_alarm_after_push()
    {
        Tango::MultiAttribute *multi_attr = Base::get_device_attr();
        attr_value = k_alarming_value;
        Base::push_change_event("attr", &attr_value);
        return multi_attr->check_alarm("attr");
    }

    bool has_alarm_after_set()
    {
        Tango::MultiAttribute *multi_attr = Base::get_device_attr();
        Tango::Attribute &attr = multi_attr->get_attr_by_name("attr");
        attr_value = k_alarming_value;
        attr.set_value(&attr_value);
        return multi_attr->check_alarm("attr");
    }

    bool has_alarm_after_force()
    {
        Tango::MultiAttribute *multi_attr = Base::get_device_attr();
        Tango::Attribute &attr = multi_attr->get_attr_by_name("attr");
        attr.set_value_date_quality(
            &attr_value, Tango::make_TimeVal(std::chrono::steady_clock::now()), Tango::ATTR_ALARM);
        return multi_attr->check_alarm("attr");
    }

    bool has_alarm_after_second_check()
    {
        Tango::MultiAttribute *multi_attr = Base::get_device_attr();
        Tango::Attribute &attr = multi_attr->get_attr_by_name("attr");
        attr_value = k_alarming_value;
        attr.set_value(&attr_value);
        multi_attr->check_alarm("attr");
        return multi_attr->check_alarm("attr");
    }

    bool has_alarm_after_push_except()
    {
        Tango::MultiAttribute *multi_attr = Base::get_device_attr();
        try
        {
            TANGO_THROW_EXCEPTION(k_test_reason, k_a_helpful_desc);
        }
        catch(Tango::DevFailed &ex)
        {
            Base::push_change_event("attr", &ex);
        }

        return multi_attr->check_alarm("attr");
    }

    void read_attribute(Tango::Attribute &attr)
    {
        attr.set_value(&attr_value);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        Tango::UserDefaultAttrProp props;
        props.set_max_alarm(std::to_string(k_alarm_level).c_str());
        props.set_abs_change("0.1");

        attrs.push_back(new TangoTest::AutoAttr<&AlarmDev::read_attribute>("attr", Tango::DEV_DOUBLE));
        attrs.back()->set_default_properties(props);
        attrs.back()->set_change_event(true, true);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&AlarmDev::has_alarm_after_set>("has_alarm_after_set"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmDev::has_alarm_after_push>("has_alarm_after_push"));
        cmds.push_back(
            new TangoTest::AutoCommand<&AlarmDev::has_alarm_after_push_except>("has_alarm_after_push_except"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmDev::has_alarm_after_force>("has_alarm_after_force"));
        cmds.push_back(
            new TangoTest::AutoCommand<&AlarmDev::has_alarm_after_second_check>("has_alarm_after_second_check"));
    }

  private:
    Tango::DevDouble attr_value = 0.0;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AlarmDev, 1)

SCENARIO("check_alarm reports alarms correctly")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"alarm", "AlarmDev", idlver};
        std::unique_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        WHEN("we call check_alarm after setting alarming value")
        {
            Tango::DeviceData result;
            REQUIRE_NOTHROW(result = device->command_inout("has_alarm_after_set"));

            THEN("the command returns true")
            {
                using namespace TangoTest::Matchers;

                REQUIRE_THAT(result, AnyLikeContains(true));
            }
        }

        if(idlver >= 4)
        {
            WHEN("we subscribe the change events for the attribute")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event("attr", Tango::CHANGE_EVENT, &callback));

                // discard the initial events we get when we subscribe
                using namespace Catch::Matchers;
                using namespace TangoTest::Matchers;

                auto maybe_initial_event = callback.pop_next_event();
                REQUIRE(maybe_initial_event != std::nullopt);

                AND_WHEN("we call check_alarm after push_change_event")
                {
                    Tango::DeviceData result;
                    REQUIRE_NOTHROW(result = device->command_inout("has_alarm_after_push"));

                    THEN("we should receive a change event")
                    {
                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventType(Tango::CHANGE_EVENT));
                        REQUIRE_THAT(maybe_event, EventValueMatches(AttrQuality(Tango::ATTR_ALARM)));

                        AND_THEN("the command returns false")
                        {
                            using namespace TangoTest::Matchers;

                            REQUIRE_THAT(result, AnyLikeContains(false));
                        }
                    }
                }

                AND_WHEN("we call check_alarm after pushing an exception")
                {
                    Tango::DeviceData result;
                    REQUIRE_NOTHROW(result = device->command_inout("has_alarm_after_push_except"));

                    THEN("we should receive an error event")
                    {
                        using namespace Catch::Matchers;
                        using namespace TangoTest::Matchers;

                        auto maybe_event = callback.pop_next_event();

                        REQUIRE(maybe_event != std::nullopt);
                        REQUIRE_THAT(maybe_event, EventErrorMatches(AllMatch(Reason(k_test_reason))));

                        AND_THEN("the command returns false")
                        {
                            using namespace TangoTest::Matchers;

                            REQUIRE_THAT(result, AnyLikeContains(false));
                        }
                    }
                }
            }
        }

        WHEN("we call check_alarm_after push_change_event without subscribing")
        {
            Tango::DeviceData result;
            REQUIRE_NOTHROW(result = device->command_inout("has_alarm_after_push"));

            AND_THEN("the command returns false")
            {
                using namespace TangoTest::Matchers;

                REQUIRE_THAT(result, AnyLikeContains(false));
            }
        }

        WHEN("we call check_alarm after forcing the quality to alarm")
        {
            Tango::DeviceData result;
            REQUIRE_NOTHROW(result = device->command_inout("has_alarm_after_force"));

            THEN("the command returns true")
            {
                using namespace TangoTest::Matchers;

                REQUIRE_THAT(result, AnyLikeContains(true));
            }
        }

        WHEN("we call check_alarm after forcing the quality to alarm")
        {
            Tango::DeviceData result;
            REQUIRE_NOTHROW(result = device->command_inout("has_alarm_after_second_check"));

            THEN("the command returns true")
            {
                using namespace TangoTest::Matchers;

                REQUIRE_THAT(result, AnyLikeContains(true));
            }
        }
    }
}
