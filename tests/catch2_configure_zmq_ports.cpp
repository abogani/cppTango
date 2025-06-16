#include "catch2_common.h"

template <class Base>
class ZmqPorts : public Base
{
  public:
    using Base::Base;

    ~ZmqPorts() override { }

    void init_device() override
    {
        value = false;
        Base::set_change_event("attr", true, true);
    }

    void read_attr(Tango::Attribute &att) override
    {
        att.set_value(&value);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        using Attr = TangoTest::AutoAttr<&ZmqPorts::read_attr>;

        attrs.push_back(new Attr("attr", Tango::DEV_BOOLEAN));
    }

  private:
    Tango::DevBoolean value;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(ZmqPorts, 4)

SCENARIO("ZmqEventSupplier can bind to ephemeral ports")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"zmq_ports", "ZmqPorts", idlver};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a device proxy to the admin device")
        {
            auto admin = ctx.get_admin_proxy();

            WHEN("we subscribe to attribute change events")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                TangoTest::Subscription sub{device, "attr", Tango::CHANGE_EVENT, &callback};

                THEN("the admin device reports being bound to ephermal ports")
                {
                    using namespace Catch::Matchers;

                    Tango::DeviceData din, dout;
                    std::vector<std::string> din_info;
                    din_info.emplace_back("info");
                    din << din_info;
                    REQUIRE_NOTHROW(dout = admin->command_inout("ZmqEventSubscriptionChange", din));

                    const Tango::DevVarLongStringArray *result;
                    dout >> result;

                    INFO(result->svalue);
                    REQUIRE(result->svalue.length() >= 2);
                    for(CORBA::ULong i = 0; i < 2; i++)
                    {
                        std::string_view endpoint = result->svalue[i].in();
                        auto pos = endpoint.rfind(':');
                        REQUIRE(pos != std::string_view::npos);
                        std::string_view port_str = endpoint.substr(pos + 1);
                        auto port = parse_as<int>(port_str);
                        REQUIRE(port >= std::numeric_limits<std::uint16_t>::min());
                        REQUIRE(port <= std::numeric_limits<std::uint16_t>::max());
                    }
                }
            }
        }
    }
}

SCENARIO("ZmqEventSupplier can bind to specific ports")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device with ZMQ ports specified")
    {
        // These ports have to unique between all the scenarios in this file
        // to allow parallel test execution from ctest
        std::string event_port = "9977";
        std::string heartbeat_port = "9988";

        std::vector<std::string> env{
            "TANGO_ZMQ_EVENT_PORT=" + event_port,
            "TANGO_ZMQ_HEARTBEAT_PORT=" + heartbeat_port,
        };
        TangoTest::Context ctx{"zmq_ports", "ZmqPorts", idlver, std::move(env)};
        std::shared_ptr<Tango::DeviceProxy> device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a device proxy to the admin device")
        {
            auto admin = ctx.get_admin_proxy();

            WHEN("we subscribe to attribute change events")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                TangoTest::Subscription sub{device, "attr", Tango::CHANGE_EVENT, &callback};

                THEN("the admin device reports being bound to the specified ports")
                {
                    using namespace Catch::Matchers;

                    Tango::DeviceData din, dout;
                    std::vector<std::string> din_info;
                    din_info.emplace_back("info");
                    din << din_info;
                    REQUIRE_NOTHROW(dout = admin->command_inout("ZmqEventSubscriptionChange", din));

                    const Tango::DevVarLongStringArray *result;
                    dout >> result;

                    INFO(result->svalue);
                    REQUIRE(result->svalue.length() >= 2);
                    for(CORBA::ULong i = 0; i < 2; i++)
                    {
                        std::string_view endpoint = result->svalue[i].in();
                        auto pos = endpoint.rfind(':');
                        REQUIRE(pos != std::string_view::npos);
                        std::string port_str{endpoint.substr(pos + 1)};
                        if(i == 0)
                        {
                            REQUIRE(port_str == heartbeat_port);
                        }
                        else
                        {
                            REQUIRE(port_str == event_port);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("ZmqEventSupplier reports an error when event port invalid")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a IDLv" << idlver << " device with an invalid ZMQ event ports specified")
    {
        // These ports have to unique between all the scenarios in this file
        // to allow parallel test execution from ctest
        std::string event_port = "XXXX";
        std::string heartbeat_port = "9989";

        std::vector<std::string> env{
            "TANGO_ZMQ_EVENT_PORT=" + event_port,
            "TANGO_ZMQ_HEARTBEAT_PORT=" + heartbeat_port,
        };
        TangoTest::Context ctx{"zmq_ports", "ZmqPorts", idlver, std::move(env)};
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a device proxy to the admin device")
        {
            auto admin = ctx.get_admin_proxy();

            WHEN("we fail to subscribe to attribute change events")
            {
                using namespace TangoTest::Matchers;
                using namespace Catch::Matchers;

                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_THROWS_MATCHES(device->subscribe_event("attr", Tango::CHANGE_EVENT, &callback),
                                       Tango::DevFailed,
                                       FirstErrorMatches(Reason(Tango::API_ZmqInitFailed) &&
                                                         DescriptionMatches(ContainsSubstring(event_port)) &&
                                                         DescriptionMatches(ContainsSubstring(strerror(EINVAL)))) &&
                                           ErrorListMatches(AnyMatch(
                                               DescriptionMatches(ContainsSubstring("Failed to bind event socket")))));

                THEN("the admin device reports being bound to the specified ports regardless")
                {
                    using namespace Catch::Matchers;

                    Tango::DeviceData din, dout;
                    std::vector<std::string> din_info;
                    din_info.emplace_back("info");
                    din << din_info;
                    REQUIRE_NOTHROW(dout = admin->command_inout("ZmqEventSubscriptionChange", din));

                    const Tango::DevVarLongStringArray *result;
                    dout >> result;

                    INFO(result->svalue);
                    REQUIRE(result->svalue.length() >= 2);
                    for(CORBA::ULong i = 0; i < 2; i++)
                    {
                        std::string_view endpoint = result->svalue[i].in();
                        auto pos = endpoint.rfind(':');
                        REQUIRE(pos != std::string_view::npos);
                        std::string port_str{endpoint.substr(pos + 1)};
                        if(i == 0)
                        {
                            REQUIRE(port_str == heartbeat_port);
                        }
                        else
                        {
                            REQUIRE(port_str == event_port);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("ZmqEventSupplier reports an error when heartbeat port invalid")
{
    GIVEN("an invalid heartbeat port")
    {
        // These ports have to unique between all the scenarios in this file
        // to allow parallel test execution from ctest
        std::string event_port = "9978";
        std::string heartbeat_port = "YYYYY";
        WHEN("starting a device server")
        {
            auto start_server = [&]()
            {
                std::vector<std::string> env{
                    "TANGO_ZMQ_EVENT_PORT=" + event_port,
                    "TANGO_ZMQ_HEARTBEAT_PORT=" + heartbeat_port,
                };
                TangoTest::Context ctx{"zmq_ports", "ZmqPorts", 4, std::move(env)};
            };
            THEN("the device server fails to start")
            {
                using namespace Catch::Matchers;
                REQUIRE_THROWS_MATCHES(start_server(),
                                       std::runtime_error,
                                       MessageMatches(ContainsSubstring(Tango::API_ZmqInitFailed) &&
                                                      ContainsSubstring(heartbeat_port) &&
                                                      ContainsSubstring(strerror(EINVAL)) &&
                                                      ContainsSubstring("Failed to bind heartbeat socket")));
            }
        }
    }
}
