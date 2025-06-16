#include "catch2_common.h"

static constexpr double SERVER_VALUE = 8.888;

template <class Base>
class AttrProxyDev : public Base
{
  public:
    using Base::Base;

    ~AttrProxyDev() override { }

    void init_device() override { }

    void read_attribute(Tango::Attribute &att)
    {
        attr_dq_double = SERVER_VALUE;
        att.set_value_date_quality(&attr_dq_double, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&AttrProxyDev::read_attribute>("attr_dq_db", Tango::DEV_DOUBLE));
    }

  private:
    Tango::DevDouble attr_dq_double;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AttrProxyDev, 3)

SCENARIO("AttributeProxy basic functionality")
{
    int idlver = GENERATE(TangoTest::idlversion(3));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_proxy_dev", "AttrProxyDev", idlver};
        auto device = ctx.get_proxy();

        WHEN("we can create an AttributeProxy to it")
        {
            auto ap = Tango::AttributeProxy(device.get(), "attr_dq_db");

            AND_THEN("the read functions returns")
            {
                using namespace TangoTest::Matchers;

                Tango::DeviceAttribute result;
                REQUIRE_NOTHROW(result = ap.read());

                REQUIRE_THAT(result, AnyLikeContains(SERVER_VALUE));
            }

            AND_THEN("read again")
            {
                REQUIRE_NOTHROW(ctx.stop_server());

                using namespace TangoTest::Matchers;

                // read the attribute before so that we run into the rate limiting of Connection::reconnect
                Tango::DeviceAttribute result;
                REQUIRE_THROWS_MATCHES(result = device->read_attribute("attr_dq_db"),
                                       Tango::DevFailed,
                                       ErrorListMatches(AnyMatch(Reason(Tango::API_ServerNotRunning))));

                auto ap = Tango::AttributeProxy(device.get(), "attr_dq_db");
                REQUIRE_THROWS_MATCHES(result = ap.read(),
                                       Tango::DevFailed,
                                       ErrorListMatches(AnyMatch(Reason(Tango::API_CantConnectToDevice))));
            }
        }
    }
}

SCENARIO("AttributeProxy: Bails when unsubscribing without subscriptions")
{
    int idlver = GENERATE(TangoTest::idlversion(3));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_proxy_dev", "AttrProxyDev", idlver};
        auto device = ctx.get_proxy();

        WHEN("we can create an AttributeProxy to it")
        {
            using namespace TangoTest::Matchers;

            auto ap = Tango::AttributeProxy(device.get(), "attr_dq_db");

            REQUIRE_THROWS_MATCHES(ap.unsubscribe_event(4711),
                                   Tango::DevFailed,
                                   ErrorListMatches(AnyMatch(Reason(Tango::API_EventNotFound))));
        }
    }
}

SCENARIO("AttributeProxy: Bails when trying to subscribe to unknown attribute")
{
    int idlver = GENERATE(TangoTest::idlversion(3));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_proxy_dev", "AttrProxyDev", idlver};
        auto device = ctx.get_proxy();

        WHEN("we can create an AttributeProxy to it")
        {
            using namespace TangoTest::Matchers;

            REQUIRE_THROWS_MATCHES(std::make_shared<Tango::AttributeProxy>(device.get(), "unknown"),
                                   Tango::DevFailed,
                                   ErrorListMatches(AnyMatch(Reason(Tango::API_UnsupportedAttribute))));
        }
    }
}
