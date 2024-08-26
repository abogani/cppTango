#include "catch2_common.h"

namespace
{
constexpr double SERVER_VALUE = 8.888;
}

template <class Base>
class TelemetryDS : public Base
{
  public:
    using Base::Base;

    void init_device() override { }

    void read_attribute(Tango::Attribute &att)
    {
        auto span = TANGO_TELEMETRY_SPAN(TANGO_CURRENT_FUNCTION, {{"myKey", "myValue"}});
        auto scope = TANGO_TELEMETRY_SCOPE(span);

        attr_dq_double = SERVER_VALUE;
        att.set_value_date_quality(&attr_dq_double, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&TelemetryDS::read_attribute>("attr_dq_db", Tango::DEV_DOUBLE));
    }

  private:
    Tango::DevDouble attr_dq_double;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(TelemetryDS, 3)

SCENARIO("Telemetry traces are outputted")
{
    int idlver = GENERATE(TangoTest::idlversion(3));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        std::vector<std::string> env{"TANGO_TELEMETRY_ENABLE=on",
                                     "TANGO_TELEMETRY_KERNEL_ENABLE=on",
                                     "TANGO_TELEMETRY_TRACES_EXPORTER=console",
                                     "TANGO_TELEMETRY_LOGS_EXPORTER=console"};
        TangoTest::Context ctx{"telemetry", "TelemetryDS", idlver, env};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        WHEN("we read the attribute")
        {
            std::string att{"attr_dq_db"};

            Tango::DeviceAttribute da;
            REQUIRE_NOTHROW(da = device->read_attribute(att));

            THEN("the read value matches the value on the server")
            {
                double att_value;
                da >> att_value;
                REQUIRE(att_value == SERVER_VALUE);
            }

            auto contents = load_file(ctx.get_redirect_file());
            REQUIRE(!contents.empty());

            using Catch::Matchers::ContainsSubstring;
            REQUIRE_THAT(contents, ContainsSubstring("code.filepath:"));

            if(idlver > 3)
            {
                REQUIRE_THAT(contents,
                             ContainsSubstring("TelemetryDS") && ContainsSubstring("read_attribute") &&
                                 ContainsSubstring("myKey") && ContainsSubstring("myValue"));
            }
        }
    }
}

SCENARIO("Telemetry does complain about invalid environment variables")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        struct TestData
        {
            std::string telemetry, kernel, traces_exporter, logs_exporter, traces_endpoint, logs_endpoint;
        };

        auto data = GENERATE(TestData{"bogus", "on", "console", "console", "cout", "cout"},
                             TestData{"on", "bogus", "console", "console", "cout", "cout"},
                             TestData{"on", "on", "bogus", "console", "cout", "cout"},
                             TestData{"on", "on", "console", "bogus", "cout", "cout"},
                             TestData{"on", "on", "console", "console", "bogus", "cout"},
                             TestData{"on", "on", "console", "console", "cout", "bogus"});

        WHEN("set the environment variables")
        {
            std::vector<std::string> env{"TANGO_TELEMETRY_ENABLE=" + data.telemetry,
                                         "TANGO_TELEMETRY_KERNEL_ENABLE=" + data.kernel,
                                         "TANGO_TELEMETRY_TRACES_EXPORTER=" + data.traces_exporter,
                                         "TANGO_TELEMETRY_LOGS_EXPORTER=" + data.logs_exporter,
                                         "TANGO_TELEMETRY_TRACES_ENDPOINT=" + data.traces_endpoint,
                                         "TANGO_TELEMETRY_LOGS_ENDPOINT=" + data.logs_endpoint};

            INFO(Catch::StringMaker<std::vector<std::string>>::convert(env));

            // avoid parsing issues in REQUIRE_XXX macro
            auto f = [&idlver, &env]() { TangoTest::Context ctx{"telemetry", "TelemetryDS", idlver, env}; };

            using Catch::Matchers::ContainsSubstring;
            using Catch::Matchers::MessageMatches;
            REQUIRE_THROWS_MATCHES(
                f(), std::runtime_error, MessageMatches(ContainsSubstring("Error reason = API_InvalidArgs")));
        }
    }
}

SCENARIO("Telemetry can be configured for all variants")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        struct TestData
        {
            std::string traces_exporter, logs_exporter, traces_endpoint, logs_endpoint;
        };

        auto data =
            GENERATE(TestData{"console", "console", "cout", "cerr"},
                     TestData{"http", "http", "http://localhost:4711/v1/traces", "https://localhost:4712/v1/traces"},
                     TestData{"grpc", "grpc", "grpc://localhost:4711", "grpc://localhost:4712"});

        WHEN("set the environment variables")
        {
            std::vector<std::string> env{"TANGO_TELEMETRY_ENABLE=ON",
                                         "TANGO_TELEMETRY_KERNEL_ENABLE=OFF",
                                         "TANGO_TELEMETRY_TRACES_EXPORTER=" + data.traces_exporter,
                                         "TANGO_TELEMETRY_LOGS_EXPORTER=" + data.logs_exporter,
                                         "TANGO_TELEMETRY_TRACES_ENDPOINT=" + data.traces_endpoint,
                                         "TANGO_TELEMETRY_LOGS_ENDPOINT=" + data.logs_endpoint};

            auto f = [&idlver, &env]() { TangoTest::Context ctx{"telemetry", "TelemetryDS", idlver, env}; };
            REQUIRE_NOTHROW(f());
        }
    }
}

SCENARIO("Telemetry traces/logs can be turned off")
{
    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        std::vector<std::string> env{"TANGO_TELEMETRY_ENABLE=on",
                                     "TANGO_TELEMETRY_KERNEL_ENABLE=on",
                                     "TANGO_TELEMETRY_TRACES_EXPORTER=none",
                                     "TANGO_TELEMETRY_LOGS_EXPORTER=none"};
        TangoTest::Context ctx{"telemetry", "TelemetryDS", idlver, env};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        WHEN("we read the attribute")
        {
            std::string att{"attr_dq_db"};

            Tango::DeviceAttribute da;
            REQUIRE_NOTHROW(da = device->read_attribute(att));

            THEN("the read value matches the value on the server")
            {
                double att_value;
                da >> att_value;
                REQUIRE(att_value == SERVER_VALUE);
            }

            auto contents = load_file(ctx.get_redirect_file());
            REQUIRE(!contents.empty());

            using Catch::Matchers::ContainsSubstring;
            REQUIRE_THAT(contents, !ContainsSubstring("code.filepath:"));
        }
    }
}
