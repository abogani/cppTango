#include "catch2_common.h"

template <class Base>
class AttrReadWrite : public Base
{
  public:
    using Base::Base;

    void init_device() override { }

    void read_attr(Tango::Attribute &att) override
    {
        if(Base::get_dev_idl_version() < 3)
        {
            // there is no write method for these IDL versions
            // tango directly writes into the internal attribute
            // so we get the value here first
            auto &watt = Base::get_device_attr()->get_w_attr_by_name("attr_dq_db");
            watt.get_write_value(attr_dq_double);
        }

        att.set_value(&attr_dq_double);
    }

    void write_attribute(Tango::WAttribute &watt)
    {
        TANGO_ASSERT(Base::get_dev_idl_version() >= 3);

        watt.get_write_value(attr_dq_double);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&AttrReadWrite::read_attr, &AttrReadWrite::write_attribute>(
            "attr_dq_db", Tango::DEV_DOUBLE));
    }

  private:
    Tango::DevDouble attr_dq_double{};
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AttrReadWrite, 1)

SCENARIO("Attributes can be read and written")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_read_write", "AttrReadWrite", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());
        WHEN("we write the attribute")
        {
            std::string att{"attr_dq_db"};
            constexpr double val = 5.678;

            Tango::DeviceAttribute da{att, val};
            REQUIRE_NOTHROW(device->write_attribute(da));

            THEN("read it back")
            {
                Tango::DeviceAttribute da;
                REQUIRE_NOTHROW(da = device->read_attribute(att));
                double val_read{};
                REQUIRE_NOTHROW(da >> val_read);

                REQUIRE(val == val_read);
            }
        }
    }
}
