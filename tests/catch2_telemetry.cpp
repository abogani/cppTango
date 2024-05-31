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
