#include "catch2_common.h"

template <class Base>
class EmptyProxy : public Base
{
  public:
    using Base::Base;

    ~EmptyProxy() override { }

    void init_device() override { }

    static void command_factory(std::vector<Tango::Command *> &) { }
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(EmptyProxy, 1)

SCENARIO("DeviceProxy can be copied")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"emptyproxy", "EmptyProxy", idlver};
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());
        THEN("The device proxy can be copied with the copy constructor")
        {
            REQUIRE_NOTHROW(Tango::DeviceProxy(*device));
            GIVEN("A device proxy that was copied")
            {
                Tango::DeviceProxy copy(*device);
                THEN("The device proxies point to the same device")
                {
                    using namespace Catch::Matchers;
                    REQUIRE(copy.get_idl_version() == idlver);
                    REQUIRE_THAT(copy.name(), Equals(device->name()));
                }
            }
        }
        THEN("The device proxy can be copied with the assignment operator")
        {
            Tango::DeviceProxy copy;
            REQUIRE_NOTHROW(copy = *device);
            AND_THEN("The device proxies point to the same device")
            {
                using namespace Catch::Matchers;
                REQUIRE(copy.get_idl_version() == idlver);
                REQUIRE_THAT(copy.name(), Equals(device->name()));
            }
        }
    }
}
