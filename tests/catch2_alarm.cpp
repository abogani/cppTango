#include "catch2_common.h"

constexpr static const Tango::DevDouble k_alarm_level = 20;
constexpr static const Tango::DevDouble k_alarming_value = 99;

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

    void read_attribute(Tango::Attribute &attr)
    {
        attr.set_value(&attr_value);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        Tango::UserDefaultAttrProp props;
        props.set_max_alarm(std::to_string(k_alarm_level).c_str());

        attrs.push_back(new TangoTest::AutoAttr<&AlarmDev::read_attribute>("attr", Tango::DEV_DOUBLE));
        attrs.back()->set_default_properties(props);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&AlarmDev::has_alarm_after_set>("has_alarm_after_set"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmDev::has_alarm_after_push>("has_alarm_after_push"));
        cmds.push_back(new TangoTest::AutoCommand<&AlarmDev::has_alarm_after_force>("has_alarm_after_force"));
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
                using TangoTest::AnyLikeContains;

                REQUIRE_THAT(result, AnyLikeContains(true));
            }
        }

        // The behaviour of these next two when blocks is a little surprising,
        // but this is how cppTango currently works (I would expect the command
        // to return `true` in both cases).
        //
        // TODO(#1182): Determine if this is really what we want
        WHEN("we call check_alarm after push_change_event")
        {
            Tango::DeviceData result;
            REQUIRE_NOTHROW(result = device->command_inout("has_alarm_after_push"));

            THEN("the command returns false")
            {
                using TangoTest::AnyLikeContains;

                REQUIRE_THAT(result, AnyLikeContains(false));
            }
        }

        WHEN("we call check_alarm after forcing the quality to alarm")
        {
            Tango::DeviceData result;
            REQUIRE_NOTHROW(result = device->command_inout("has_alarm_after_force"));

            THEN("the command returns false")
            {
                using TangoTest::AnyLikeContains;

                REQUIRE_THAT(result, AnyLikeContains(false));
            }
        }
    }
}
