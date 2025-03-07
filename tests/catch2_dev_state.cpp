#include "catch2_common.h"

constexpr static const Tango::DevDouble k_alarm_level = 20;
constexpr static const Tango::DevDouble k_alarming_value = 99;
constexpr static const Tango::DevDouble k_normal_value = 0;

constexpr static const char *k_test_reason = "Test_Reason";
constexpr static const char *k_a_helpful_desc = "A helpful description";

enum class Action
{
    normal,
    alarm,
    except,

};

Action action_from_string(std::string_view str)
{
    if(str == "normal")
    {
        return Action::normal;
    }

    if(str == "alarm")
    {
        return Action::alarm;
    }

    if(str == "except")
    {
        return Action::except;
    }

    std::stringstream ss;
    ss << "Unknown action \"" << str << "\"";
    TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, ss.str());
}

std::string to_string(Action action)
{
    switch(action)
    {
    case Action::normal:
        return "normal";
    case Action::alarm:
        return "alarm";
    case Action::except:
        return "except";
    default:
        return "unknown";
    }
}

template <class Base>
class DevStateExcept : public Base
{
  public:
    using Base::Base;

    ~DevStateExcept() override { }

    void init_device() override
    {
        Base::set_state(Tango::ON);

        Tango::MultiAttribute *multi_attr = Base::get_device_attr();
        std::vector<Tango::Attribute *> attrs = multi_attr->get_attribute_list();

        m_on_read.clear();
        for(auto *attr : attrs)
        {
            const std::string &name = attr->get_name();
            if(name.find("attr") == 0)
            {
                m_on_read[name] = Action::normal;
                m_values[name] = k_normal_value;
            }
        }
    }

    Tango::DevState dev_state() override
    {
        Tango::DevState state = Base::dev_state();

        m_dev_state_called = true;

        return state;
    }

    void reset_dev_state_called()
    {
        m_dev_state_called = false;
    }

    void read_attr(Tango::Attribute &attr) override
    {
        Action action = m_on_read[attr.get_name()];
        TANGO_LOG_DEBUG << "Performing action " << to_string(action) << " for " << attr.get_name();
        switch(action)
        {
        case Action::normal:
        {
            Tango::DevDouble &value = m_values[attr.get_name()];
            value = k_normal_value;
            attr.set_value(&value);
            break;
        }
        case Action::alarm:
        {
            Tango::DevDouble &value = m_values[attr.get_name()];
            value = k_alarming_value;
            attr.set_value(&value);
            break;
        }
        case Action::except:
            TANGO_THROW_EXCEPTION(k_test_reason, k_a_helpful_desc);
        default:
            TANGO_ASSERT_ON_DEFAULT(action);
        }
    }

    bool has_dev_state_been_called()
    {
        return m_dev_state_called;
    }

    void set_actions(const Tango::DevVarStringArray &args)
    {
        if(args.length() != 3)
        {
            std::stringstream ss;
            ss << "length (= " << args.length() << ") != 3";
            TANGO_THROW_EXCEPTION(Tango::API_InvalidArgs, ss.str());
        }

        for(Tango::DevULong i = 0; i < args.length(); ++i)
        {
            std::string attr_name = [](Tango::DevULong num)
            {
                std::stringstream ss;
                ss << "attr" << num;
                return ss.str();
            }(i + 1);

            TANGO_LOG_DEBUG << "Setting action " << args[i].in() << " for " << attr_name;

            Action action = action_from_string(args[i].in());

            m_on_read[attr_name] = action;
        }
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        Tango::UserDefaultAttrProp props;
        props.set_max_alarm(std::to_string(k_alarm_level).c_str());

        attrs.push_back(new TangoTest::AutoAttr<&DevStateExcept::read_attr>("attr1", Tango::DEV_DOUBLE));
        attrs.back()->set_default_properties(props);

        attrs.push_back(new TangoTest::AutoAttr<&DevStateExcept::read_attr>("attr2", Tango::DEV_DOUBLE));
        attrs.back()->set_default_properties(props);

        attrs.push_back(new TangoTest::AutoAttr<&DevStateExcept::read_attr>("attr3", Tango::DEV_DOUBLE));
        attrs.back()->set_default_properties(props);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&DevStateExcept::set_actions>("set_actions"));
        cmds.push_back(
            new TangoTest::AutoCommand<&DevStateExcept::has_dev_state_been_called>("has_dev_state_been_called"));
        cmds.push_back(new TangoTest::AutoCommand<&DevStateExcept::reset_dev_state_called>("reset_dev_state_called"));
    }

  private:
    std::unordered_map<std::string, Action> m_on_read;
    std::unordered_map<std::string, Tango::DevDouble> m_values;
    bool m_dev_state_called = false;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(DevStateExcept, 1)

SCENARIO("dev_state works with exceptions")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"state", "DevStateExcept", idlver};
        auto device = ctx.get_proxy();

        struct TestData
        {
            std::array<const char *, 3> actions;
            Tango::DevState expected;
        };

        auto data = GENERATE(TestData{{"normal", "normal", "normal"}, Tango::ON},
                             TestData{{"normal", "except", "normal"}, Tango::ON},
                             TestData{{"alarm", "normal", "normal"}, Tango::ALARM},
                             TestData{{"alarm", "except", "normal"}, Tango::ALARM},
                             TestData{{"normal", "except", "alarm"}, Tango::ALARM});

        WHEN("we prime attributes with " << Catch::StringMaker<decltype(data.actions)>::convert(data.actions)
                                         << " actions")
        {
            Tango::DeviceData args;
            Tango::DevVarStringArray actions;

            actions.length(3);

            actions[0] = data.actions[0];
            actions[1] = data.actions[1];
            actions[2] = data.actions[2];

            args << actions;

            REQUIRE_NOTHROW(device->command_inout("set_actions", args));

            AND_WHEN("we call the State command")
            {
                Tango::DeviceData dd;
                REQUIRE_NOTHROW(dd = device->command_inout("State"));

                THEN("we find an " << Tango::DevStateName[data.expected] << " state")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THAT(dd, AnyLikeContains(data.expected));
                }
            }

            // Reading the "State" attribute is only available from IDLv3
            // onwards.
            if(idlver >= 3)
            {
                AND_WHEN("we read the State attribute")
                {
                    Tango::DeviceAttribute da;
                    REQUIRE_NOTHROW(da = device->read_attribute("State"));

                    THEN("we find an " << Tango::DevStateName[data.expected] << " state")
                    {
                        using namespace TangoTest::Matchers;

                        REQUIRE_THAT(da, AnyLikeContains(data.expected));
                    }
                }
            }
        }
    }
}

SCENARIO("user dev_state is always called")
{
    int idlver = GENERATE(TangoTest::idlversion(3));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device in ALARM state")
    {
        using namespace TangoTest::Matchers;

        TangoTest::Context ctx{"state", "DevStateExcept", idlver};
        auto device = ctx.get_proxy();

        Tango::DeviceData args;
        Tango::DevVarStringArray actions;

        actions.length(3);

        actions[0] = "alarm";
        actions[1] = "normal";
        actions[2] = "normal";

        args << actions;

        REQUIRE_NOTHROW(device->command_inout("set_actions", args));
        Tango::DeviceData dd;
        REQUIRE_NOTHROW(dd = device->command_inout("State"));
        REQUIRE_THAT(dd, AnyLikeContains(Tango::ALARM));
        REQUIRE_NOTHROW(dd = device->command_inout("reset_dev_state_called"));

        WHEN("we prime the alarming attribute to throw an exception")
        {
            actions[0] = "except";
            actions[1] = "normal";
            actions[2] = "normal";
            args << actions;
            REQUIRE_NOTHROW(device->command_inout("set_actions", args));

            AND_WHEN("we read the throwing attribute and State")
            {
                using UniquePtr = std::unique_ptr<std::vector<Tango::DeviceAttribute>>;
                using namespace TangoTest::Matchers;

                UniquePtr das;
                REQUIRE_NOTHROW(das = UniquePtr{device->read_attributes({"attr1", "State"})});

                THEN("it returns an exception for the throwing attribute and ON for the State")
                {
                    Tango::DevDouble value;
                    REQUIRE_THROWS_MATCHES(
                        (*das)[0] >> value, Tango::DevFailed, ErrorListMatches(AnyMatch(Reason(k_test_reason))));

                    REQUIRE_THAT((*das)[1], AnyLikeContains(Tango::ALARM));

                    AND_THEN("we find that dev_state() has been called")
                    {
                        using namespace TangoTest::Matchers;

                        Tango::DeviceData dd;
                        REQUIRE_NOTHROW(dd = device->command_inout("has_dev_state_been_called"));
                        REQUIRE_THAT(dd, AnyLikeContains(true));
                    }
                }
            }
        }
    }
}
