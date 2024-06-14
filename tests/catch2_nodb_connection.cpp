#include "catch2_common.h"

SCENARIO("Connection to invalid nodb device name")
{
    REQUIRE_NOTHROW(Tango::DeviceProxy("tango://localhost:0/invalid/test/dev#dbase=no"));
}

template <class Base>
class DoubleROAttrServer : public Base
{
  public:
    using Base::Base;

    Tango::DevDouble *attr_double_value_read;
    constexpr static Tango::DevDouble SIMPLE_SERVER_DOUBLE_VALUE = 42.1234;

    ~DoubleROAttrServer() override
    {
        std::cout << "DoubleROAttrServer: in destructor" << std::endl;
        delete_device();
    }

    void delete_device() override
    {
        std::cout << "DoubleROAttrServer: in delete_device()" << std::endl;
        delete[] attr_double_value_read;
    }

    void init_device() override
    {
        // at server initialization
        std::cout << "DoubleROAttrServer: in init_device" << std::endl;
        attr_double_value_read = new Tango::DevDouble[1];
        *attr_double_value_read = SIMPLE_SERVER_DOUBLE_VALUE;
    }

    void read_attribute(Tango::Attribute &attr)
    {
        std::cout << "DoubleROAttrServer: Reading attribute... " << std::endl;
        attr.set_value(attr_double_value_read);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(
            new TangoTest::AutoAttr<&DoubleROAttrServer::read_attribute>("double_value", Tango::DEV_DOUBLE));
    }
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(DoubleROAttrServer, 3)

SCENARIO("Test connection and reading a double RO attribute on a nodb device")
{
    int idlver = GENERATE(TangoTest::idlversion(3));
    GIVEN("a device proxy to a IDLv" << idlver << " nodb device")
    {
        TangoTest::Context ctx{"no_db_connection", "DoubleROAttrServer", idlver};
        auto dp_ptr = ctx.get_proxy();
        REQUIRE(idlver == dp_ptr->get_idl_version());
        Tango::DevDouble double_val = 0.0;

        THEN("we can ping the device")
        {
            REQUIRE_NOTHROW(dp_ptr->ping());
            AND_THEN("we can read the double_value attribute")
            {
                REQUIRE_NOTHROW(
                    [&]()
                    {
                        auto da = dp_ptr->read_attribute("double_value");
                        da >> double_val;
                    }());
                REQUIRE(double_val == DoubleROAttrServer<TANGO_BASE_CLASS>::SIMPLE_SERVER_DOUBLE_VALUE);
            }
        }

        AND_WHEN("we stop the server")
        {
            REQUIRE_NOTHROW(ctx.stop_server());
            double_val = 0.0;
            THEN("We can no longer read the double_value attribute")
            {
                using TangoTest::FirstErrorMatches, TangoTest::Reason;

                REQUIRE_THROWS_MATCHES(
                    [&]()
                    {
                        auto da = dp_ptr->read_attribute("double_value");
                        da >> double_val;
                    }(),
                    Tango::DevFailed,
                    FirstErrorMatches(Reason(Tango::API_CorbaException)));
            }
            REQUIRE(double_val == 0.0);
        }
    }
}
