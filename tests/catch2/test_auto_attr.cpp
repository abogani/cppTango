#include <tango/tango.h>

#include <memory>

#include "utils/utils.h"

static constexpr double ATTR_TEST_VALUE = 42.0;

// Test device class
template <class Base>
class AutoAttrDev : public Base
{
  public:
    using Base::Base;

    ~AutoAttrDev() override { }

    void init_device() override
    {
        value = 0;
    }

    void read_value(Tango::Attribute &att)
    {
        att.set_value_date_quality(&value, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    void write_value(Tango::WAttribute &att)
    {
        att.get_write_value(value);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(
            new TangoTest::AutoAttr<&AutoAttrDev::read_value, &AutoAttrDev::write_value>("value", Tango::DEV_DOUBLE));
    }

  private:
    Tango::DevDouble value;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AutoAttrDev, 3)

SCENARIO("AutoAttr can be read and written")
{
    int idlver = GENERATE(range(3, 7));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"auto_attr", "AutoAttrDev", idlver};
        INFO(ctx.info());
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("an attribute name")
        {
            std::string att{"value"};

            WHEN("we write the attribute")
            {
                Tango::DeviceAttribute in;
                in.set_name(att);
                in << ATTR_TEST_VALUE;
                REQUIRE_NOTHROW(device->write_attribute(in));

                THEN("we read back matching value")
                {
                    Tango::DeviceAttribute out;
                    REQUIRE_NOTHROW(out = device->read_attribute(att));
                    double value;
                    out >> value;
                    REQUIRE(value == ATTR_TEST_VALUE);
                }
            }
        }
    }
}
