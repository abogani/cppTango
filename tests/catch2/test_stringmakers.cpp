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

template <>
Tango::CommandInfo GetDefault()
{
    Tango::CommandInfo var;
    var.disp_level = Tango::DL_UNKNOWN;
    var.in_type = Tango::DEV_VOID;
    var.out_type = Tango::DEV_VOID;

    return var;
}

template <>
Tango::AttributeInfoEx GetDefault()
{
    Tango::AttributeInfoEx var;
    var.memorized = Tango::NOT_KNOWN;
    var.disp_level = Tango::DL_UNKNOWN;
    var.writable = Tango::WT_UNKNOWN;
    var.data_format = Tango::FMT_UNKNOWN;
    var.data_type = Tango::DEV_VOID;

    return var;
}

template <>
Tango::DevIntrChangeEventData GetDefault()
{
    Tango::DevIntrChangeEventData var;

    var.cmd_list.emplace_back(GetDefault<Tango::CommandInfo>());
    var.att_list.emplace_back(GetDefault<Tango::AttributeInfoEx>());

    return var;
}

template <>
Tango::TimeVal GetDefault()
{
    Tango::TimeVal val;
    val.tv_sec = 1;
    val.tv_usec = 2;
    val.tv_nsec = 3;

    return val;
}

template <>
TangoTest::AttrReadEventCopyable GetDefault()
{
    std::vector<std::string> att_names;
    Tango::DevErrorList errors;
    Tango::AttrReadEvent event(nullptr, att_names, nullptr, errors);
    TangoTest::AttrReadEventCopyable var(&event);

    return var;
}

template <>
TangoTest::AttrWrittenEventCopyable GetDefault()
{
    std::vector<std::string> att_names;
    Tango::NamedDevFailedList errors;
    Tango::AttrWrittenEvent event(nullptr, att_names, errors);
    TangoTest::AttrWrittenEventCopyable var(&event);

    return var;
}

template <>
TangoTest::CmdDoneEventCopyable GetDefault()
{
    std::string cmd_name;
    Tango::DevErrorList errors;
    Tango::DeviceData argout;
    Tango::CmdDoneEvent event(nullptr, cmd_name, argout, errors);
    TangoTest::CmdDoneEventCopyable var(&event);

    return var;
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

SCENARIO("catch2 stringmakers specialications (non-standard)")
{
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

// clang-format off
using PrintableType = std::tuple<
                                 Tango::ArchiveEventInfo,
                                 Tango::AttributeAlarmInfo,
                                 Tango::AttributeEventInfo,
                                 Tango::AttributeInfoEx,
                                 Tango::AttrConfEventData,
                                 Tango::ChangeEventInfo,
                                 Tango::CommandInfo,
                                 Tango::DataReadyEventData,
                                 Tango::DevIntrChangeEventData,
                                 Tango::DeviceAttribute,
                                 Tango::DeviceData,
                                 Tango::DeviceInfo,
                                 Tango::EventData,
                                 Tango::FwdAttrConfEventData,
                                 Tango::FwdEventData,
                                 Tango::PeriodicEventInfo,
                                 Tango::PipeEventData,
                                 Tango::TimeVal,
                                 TangoTest::AttrReadEventCopyable,
                                 TangoTest::AttrWrittenEventCopyable,
                                 TangoTest::CmdDoneEventCopyable,
                                 // IDL classes
                                 Tango::AttributeDim,
                                 Tango::AttributeValue_5,
                                 Tango::AttributeConfig_5,
                                 Tango::ArchiveEventProp,
                                 Tango::AttributeAlarm,
                                 Tango::ChangeEventProp,
                                 Tango::EventProperties,
                                 Tango::PeriodicEventProp
                                 >;
// clang-format on

TEMPLATE_LIST_TEST_CASE("catch2 stringmakers specialications", "", PrintableType)
{
    auto var = GetDefault<TestType>();
    using namespace Catch::Matchers;

    auto result = Catch::StringMaker<TestType>::convert(var);
    REQUIRE_THAT(result, !IsEmpty());
}
