#include <tango/tango.h>

#include "utils/utils.h"

#include <catch2/catch_template_test_macros.hpp>

namespace
{

// return a instance of a class suitable for printing
template <typename T>
T GetDefault()
{
    return T{};
}

template <>
Tango::DeviceAttribute GetDefault()
{
    Tango::DeviceAttribute da;

    auto errors = new Tango::DevErrorList;
    errors->length(1);
    (*errors)[0].severity = Tango::WARN;
    da.set_error_list(errors);

    return da;
}

} // anonymous namespace

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

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::EventData>::convert(evData);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    GIVEN("a AttrReadEventCopyable")
    {
        std::vector<std::string> att_names;
        Tango::DevErrorList errors;
        Tango::AttrReadEvent event(nullptr, att_names, nullptr, errors);
        TangoTest::AttrReadEventCopyable event_copyable(&event);

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<TangoTest::AttrReadEventCopyable>::convert(event_copyable);
            REQUIRE_THAT(result, !IsEmpty());
        }
    }

    GIVEN("a DataReadyEventData")
    {
        Tango::DataReadyEventData ready_data;

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::DataReadyEventData>::convert(ready_data);
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
        auto var = GetDefault<Tango::DeviceAttribute>();

        WHEN("we can convert it to a string")
        {
            using namespace Catch::Matchers;

            auto result = Catch::StringMaker<Tango::DeviceAttribute>::convert(var);
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

    GIVEN("a DevError")
    {
        using namespace Catch::Matchers;

        Tango::DevError err;
        err.severity = Tango::WARN;

        auto result = Catch::StringMaker<Tango::DevError>::convert(err);
        REQUIRE_THAT(result, !IsEmpty());
    }

    GIVEN("a DevError_var")
    {
        using namespace Catch::Matchers;

        auto err = new Tango::DevError;
        err->severity = Tango::WARN;

        Tango::DevError_var var = err;

        auto result = Catch::StringMaker<Tango::DevError>::convert(var);
        REQUIRE_THAT(result, !IsEmpty());
    }

    GIVEN("a DevErrorList")
    {
        using namespace Catch::Matchers;

        Tango::DevErrorList err_list;
        err_list.length(2);
        err_list[0].severity = Tango::WARN;
        err_list[1].severity = Tango::PANIC;

        auto result = Catch::StringMaker<Tango::DevErrorList>::convert(err_list);
        REQUIRE_THAT(result, !IsEmpty());
    }

    GIVEN("a DevErrorList_var")
    {
        using namespace Catch::Matchers;

        auto err_list = new Tango::DevErrorList;
        err_list->length(1);
        (*err_list)[0].severity = Tango::WARN;

        Tango::DevErrorList_var var = err_list;

        auto result = Catch::StringMaker<Tango::DevErrorList>::convert(var);
        REQUIRE_THAT(result, !IsEmpty());
    }
}
