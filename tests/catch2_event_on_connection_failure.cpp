#include "catch2_common.h"

#include <tango/internal/stl_corba_helpers.h>

namespace
{
using CallbackMockType = TangoTest::CallbackMock<Tango::EventData>;

constexpr Tango::DevShort k_initial_short = 4711;

} // anonymous namespace

template <class Base>
class EventFailureDev : public Base
{
  public:
    using Base::Base;

    void init_device() override { }

    void read_attribute(Tango::Attribute &att)
    {
        att.set_value(&short_value);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        auto short_attr = new TangoTest::AutoAttr<&EventFailureDev::read_attribute>("Short_attr", Tango::DEV_SHORT);
        short_attr->set_change_event(true, false);
        attrs.push_back(short_attr);
    }

  private:
    Tango::DevShort short_value{k_initial_short};
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(EventFailureDev, 4)

SCENARIO("Event connection failure with stateless is reported during late subscription (callback)")
{
    int idlver = GENERATE(TangoTest::idlversion(Tango::MIN_IDL_ZMQ_EVENT));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"efd", "EventFailureDev", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        std::string attr_name{"Short_attr"};

        AND_GIVEN("we stop the DS and subscribe to a change event (stateless)")
        {
            REQUIRE_NOTHROW(ctx.stop_server());

            CallbackMockType cb;
            TangoTest::Subscription sub{device, attr_name, Tango::CHANGE_EVENT, &cb, true};

            AND_THEN("wait until the resubscribe period has elapsed")
            {
                std::this_thread::sleep_for(std::chrono::seconds(Tango::EVENT_HEARTBEAT_PERIOD + 1));

                AND_THEN("we should get a connection failure event")
                {
                    using namespace Catch::Matchers;
                    using namespace TangoTest::Matchers;

                    auto event = cb.pop_next_event();
                    REQUIRE(event != std::nullopt);
                    REQUIRE_THAT(event->errors, !IsEmpty() && AllMatch(Reason(Tango::API_CantConnectToDevice)));
                    REQUIRE_THAT(event, AttrNameContains(attr_name));
                    REQUIRE_THAT(event, EventType(Tango::CHANGE_EVENT));
                }
            }
        }
    }
}
