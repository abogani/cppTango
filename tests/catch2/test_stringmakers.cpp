#include <tango/tango.h>

#include "utils/utils.h"

template <class Base>
class EmptyDS : public Base
{
  public:
    using Base::Base;

    void init_device() override { }
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(EmptyDS, 1)

SCENARIO("catch2 stringmakers specialications")
{
    GIVEN("a TimeVal")
    {
        Tango::TimeVal val;
        val.tv_sec = 1;
        val.tv_usec = 2;
        val.tv_nsec = 3;

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::TimeVal>::convert(val);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    GIVEN("a EventData")
    {
        Tango::EventData evData;
        evData.device = nullptr;
        evData.attr_value = nullptr;
        evData.err = false;

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::EventData>::convert(evData);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    GIVEN("a DeviceInfo")
    {
        Tango::DeviceInfo info;

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::DeviceInfo>::convert(info);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    GIVEN("a DeviceData")
    {
        Tango::DeviceData da;

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::DeviceData>::convert(da);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    int idlver = GENERATE(TangoTest::idlversion(6));
    GIVEN("a DeviceProxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"stringmakers", "EmptyDS", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::DeviceProxy>::convert(*device);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    GIVEN("a DeviceAttribute")
    {
        Tango::DeviceAttribute da;

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::DeviceAttribute>::convert(da);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    GIVEN("a CORBA::Any")
    {
        using namespace Catch::Matchers;
        using namespace TangoTest::Matchers;

        CORBA::Any a;
        a <<= (Tango::DevDouble) 1.0;

        REQUIRE_THAT(a, AnyLikeContains(1.0));
        REQUIRE_THAT(a, AnyLikeMatches(WithinAbs(1.0, 0.0000001)));
    }

    GIVEN("a DeviceData")
    {
        using namespace Catch::Matchers;
        using namespace TangoTest::Matchers;

        Tango::DeviceData a;
        a << (Tango::DevDouble) 1.0;

        REQUIRE_THAT(a, AnyLikeContains(1.0));
        REQUIRE_THAT(a, AnyLikeMatches(WithinAbs(1.0, 0.0000001)));
    }

    GIVEN("a DeviceAttribute")
    {
        using namespace Catch::Matchers;
        using namespace TangoTest::Matchers;

        Tango::DeviceAttribute a;
        a << (Tango::DevDouble) 1.0;

        REQUIRE_THAT(a, AnyLikeContains(1.0));
        REQUIRE_THAT(a, AnyLikeMatches(WithinAbs(1.0, 0.0000001)));
    }
}
