#include <tango/tango.h>

#include <memory>

#include "utils/utils.h"

static constexpr double SERVER_VALUE = 8.888;

template <class Base>
class AttrManipDev : public Base
{
  public:
    using Base::Base;

    ~AttrManipDev() override { }

    void init_device() override { }

    void read_attribute(Tango::Attribute &att)
    {
        attr_dq_double = SERVER_VALUE;
        att.set_value_date_quality(&attr_dq_double, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&AttrManipDev::read_attribute>("attr_dq_db", Tango::DEV_DOUBLE));
    }

  private:
    Tango::DevDouble attr_dq_double;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AttrManipDev)

SCENARIO("attribute formatting can be controlled")
{
    GIVEN("a device proxy to a simple device")
    {
        TangoTest::Context ctx{"attr_manip", "AttrManipDev"};
        INFO(ctx.info());
        auto device = ctx.get_proxy();

        AND_GIVEN("an attribute name and configuration")
        {
            std::string att{"attr_dq_db"};

            Tango::AttributeInfo sta_ai;
            REQUIRE_NOTHROW(sta_ai = device->get_attribute_config(att));

            WHEN("we read the attribute")
            {
                Tango::DeviceAttribute da;
                REQUIRE_NOTHROW(da = device->read_attribute(att));

                THEN("the read value matches the value on the server")
                {
                    double att_value;
                    da >> att_value;
                    REQUIRE(att_value == SERVER_VALUE);
                }
            }

            struct TestData
            {
                const char *name;
                const char *format;
                const char *expected;
            };

            auto data = GENERATE((TestData{"scientfic", "scientific;uppercase;setprecision(2)", "8.89E+00"}),
                                 (TestData{"fixed-width", "fixed;setprecision(2)", "8.89"}));

            AND_GIVEN("a " << data.name << " format specification")
            {
                std::string format{data.format};

                WHEN("we set the attribute configuration with the format")
                {
                    Tango::AttributeInfoList new_ai;
                    new_ai.push_back(sta_ai);
                    new_ai[0].format = format;

                    REQUIRE_NOTHROW(device->set_attribute_config(new_ai));

                    THEN("the format read back matches")
                    {
                        Tango::AttributeInfo sta_ai_2;
                        REQUIRE_NOTHROW(sta_ai_2 = device->get_attribute_config(att));
                        REQUIRE(sta_ai_2.format == format);
                    }
                }

                WHEN("we format a value")
                {
                    std::stringstream out;
                    REQUIRE_NOTHROW(out << Tango::AttrManip(format) << SERVER_VALUE);

                    THEN("the rendered string is in " << data.name << " notation")
                    {
                        REQUIRE(out.str() == data.expected);
                    }
                }
            }
        }
    }
}
