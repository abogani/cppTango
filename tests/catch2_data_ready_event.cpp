#include "catch2_common.h"

#include <tango/internal/stl_corba_helpers.h>

namespace
{
using CallbackMockType = TangoTest::CallbackMock<Tango::DataReadyEventData>;

} // anonymous namespace

template <class Base>
class DataReadyDev : public Base
{
  public:
    using Base::Base;

    void init_device() override { }

    void push_data_ready(const Tango::DevVarLongStringArray &in)
    {
        Base::push_data_ready_event(in.svalue[0].in(), in.lvalue[0]);
    }

    void read_attribute(Tango::Attribute &att)
    {
        if(att.get_name() == "Long_attr")
        {
            att.set_value(&long_value);
        }
        else if(att.get_name() == "Short_attr")
        {
            att.set_value(&short_value);
        }
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        {
            auto long_attr = new TangoTest::AutoAttr<&DataReadyDev::read_attribute>("Long_attr", Tango::DEV_LONG);
            attrs.push_back(long_attr);
        }

        {
            auto short_attr = new TangoTest::AutoAttr<&DataReadyDev::read_attribute>("Short_attr", Tango::DEV_SHORT);
            Tango::UserDefaultAttrProp props;
            short_attr->set_default_properties(props);
            short_attr->set_data_ready_event(true);
            attrs.push_back(short_attr);
        }
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&DataReadyDev::push_data_ready>("PushDataReady"));
    }

  private:
    Tango::DevLong long_value{0};
    Tango::DevShort short_value{0};
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(DataReadyDev, 1)

SCENARIO("DataReadyEvent failure")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"dr", "DataReadyDev", idlver};
        auto device = ctx.get_proxy();

        std::string attr_name{"long_attr"};

        WHEN("we subscribe to the attribute " << attr_name << " without data ready event enabled")
        {
            using namespace TangoTest::Matchers;

            CallbackMockType cb;
            REQUIRE_THROWS_MATCHES(device->subscribe_event(attr_name, Tango::DATA_READY_EVENT, &cb),
                                   Tango::DevFailed,
                                   FirstErrorMatches(Reason(Tango::API_AttributeNotDataReadyEnabled)));
        }
    }
}

SCENARIO("DataReadyEvent works")
{
    // DataReadyEvent is only supported for IDLv4 and above
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"dr", "DataReadyDev", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        std::string attr_name{"Short_attr"};

        WHEN("we subscribe to the attribute " << attr_name << " supporting data ready event")
        {
            CallbackMockType cb;

            TangoTest::Subscription sub1{device, attr_name, Tango::DATA_READY_EVENT, &cb};
            TangoTest::Subscription sub2{device, attr_name, Tango::DATA_READY_EVENT, &cb};

            THEN("check that the attribute is still not polled")
            {
                REQUIRE(!device->is_attribute_polled(attr_name));
            }

            THEN("the callback should not have been executed once")
            {
                auto event = cb.pop_next_event();
                REQUIRE(event == std::nullopt);
            }

            THEN("fire a data ready event")
            {
                using namespace TangoTest::Matchers;

                const int counter = 10;

                Tango::DevVarLongStringArray dvlsa;
                dvlsa.svalue.length(1);
                dvlsa.lvalue.length(1);
                dvlsa.lvalue[0] = counter;
                dvlsa.svalue[0] = Tango::string_dup(attr_name.c_str());
                Tango::DeviceData in;
                in << dvlsa;
                REQUIRE_NOTHROW(device->command_inout("PushDataReady", in));

                auto event1 = cb.pop_next_event();
                REQUIRE(event1 != std::nullopt);
                REQUIRE_THAT(event1, EventType(Tango::DATA_READY_EVENT));
                REQUIRE_THAT(event1, EventCounter(counter));
                REQUIRE_THAT(event1, EventAttrType(Tango::DEV_SHORT));

                auto event2 = cb.pop_next_event();
                REQUIRE(event2 != std::nullopt);
                REQUIRE_THAT(event2, EventType(Tango::DATA_READY_EVENT));
                REQUIRE_THAT(event2, EventCounter(counter));
                REQUIRE_THAT(event2, EventAttrType(Tango::DEV_SHORT));
            }

            THEN("push a couple more events")
            {
                using namespace Catch::Matchers;
                using namespace TangoTest::Matchers;

                Tango::DevVarLongStringArray dvlsa;
                dvlsa.svalue.length(1);
                dvlsa.lvalue.length(1);
                dvlsa.svalue[0] = Tango::string_dup(attr_name.c_str());
                Tango::DeviceData in;

                dvlsa.lvalue[0] = 15;
                in << dvlsa;
                device->command_inout("PushDataReady", in);

                dvlsa.lvalue[0] = 16;
                in << dvlsa;
                device->command_inout("PushDataReady", in);

                dvlsa.lvalue[0] = 17;
                in << dvlsa;
                device->command_inout("PushDataReady", in);

                auto events = cb.pop_events();

                REQUIRE_THAT(events, SizeIs(6));

                auto event = events.back();
                REQUIRE(event != std::nullopt);
                REQUIRE_THAT(event, EventType(Tango::DATA_READY_EVENT));
                REQUIRE_THAT(event, EventCounter(17));
                REQUIRE_THAT(event, EventAttrType(Tango::DEV_SHORT));
            }
        }
    }
}

SCENARIO("push_data_ready_event on non-existing attribute")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"dr", "DataReadyDev", idlver};
        auto device = ctx.get_proxy();

        WHEN("fire a data ready event")
        {
            using namespace Catch::Matchers;
            using namespace TangoTest::Matchers;

            Tango::DevVarLongStringArray dvlsa;
            dvlsa.svalue.length(1);
            dvlsa.lvalue.length(1);
            dvlsa.svalue[0] = Tango::string_dup("bidon");
            Tango::DeviceData in;
            in << dvlsa;

            REQUIRE_THROWS_MATCHES(device->command_inout("PushDataReady", in),
                                   Tango::DevFailed,
                                   FirstErrorMatches(Reason(Tango::API_AttrNotFound)));
        }
    }
}
