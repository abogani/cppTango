#include "catch2_common.h"
#include "tango/server/except.h"

namespace
{

constexpr double ATTR_INIT_VALUE_DP = 9.999;
constexpr short ATTR_INIT_VALUE_SH = 4711;
const static std::string TestExceptReason = "Ahhh!";

using AttrReadCallbackMockType = TangoTest::CallbackMock<Tango::AttrReadEvent, TangoTest::AttrReadEventCopyable>;

using AttrWrittenEventCallbackMockType =
    TangoTest::CallbackMock<Tango::AttrWrittenEvent, TangoTest::AttrWrittenEventCopyable>;

using CmdDoneEventCallbackMockType = TangoTest::CallbackMock<Tango::CmdDoneEvent, TangoTest::CmdDoneEventCopyable>;

std::function<void()> get_poll_func(std::unique_ptr<Tango::DeviceProxy> &device, long timeout)
{
    if(timeout < 0)
    {
        return [&device]() { device->get_asynch_replies(); };
    }
    else
    {
        return [&device, timeout]() { device->get_asynch_replies(timeout); };
    }
}

} // anonymous namespace

template <class Base>
class AsyncAttrDev : public Base
{
  public:
    using Base::Base;

    ~AsyncAttrDev() override { }

    void init_device() override { }

    void read_attr(Tango::Attribute &att) override
    {
        if(att.get_name() == "attr_asyn")
        {
            attr_asyn = ATTR_INIT_VALUE_DP;
            att.set_value(&attr_asyn);
        }
        else if(att.get_name() == "Short_attr")
        {
            short_attr = ATTR_INIT_VALUE_SH;
            att.set_value(&short_attr);
        }
        else if(att.get_name() == "attr_asyn_to")
        {
            // sleep here intentionally so that the "CORBA read" times out
            std::this_thread::sleep_for(std::chrono::seconds(1));
            attr_asyn = ATTR_INIT_VALUE_DP;
            att.set_value(&attr_asyn);
        }
        else if(att.get_name() == "attr_asyn_except")
        {
            TANGO_THROW_EXCEPTION(TestExceptReason, "This is a test");
        }
        else
        {
            INFO("Missing case for attribute" << att.get_name());
            REQUIRE(false);
        }
    }

    void write_attr(TANGO_UNUSED(Tango::WAttribute &att))
    {
        ; // nothing for now
    }

    double identity_double_cmd(double v)
    {
        return v;
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&AsyncAttrDev::read_attr, &AsyncAttrDev::write_attr>(
            "attr_asyn", Tango::DEV_DOUBLE));
        attrs.push_back(new TangoTest::AutoAttr<&AsyncAttrDev::read_attr>("Short_attr", Tango::DEV_SHORT));
        attrs.push_back(new TangoTest::AutoAttr<&AsyncAttrDev::read_attr>("attr_asyn_except", Tango::DEV_DOUBLE));
        attrs.push_back(new TangoTest::AutoAttr<&AsyncAttrDev::read_attr>("attr_asyn_to", Tango::DEV_DOUBLE));
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&AsyncAttrDev::identity_double_cmd>("identity_double_cmd"));
    }

  private:
    Tango::DevDouble attr_asyn;
    Tango::DevShort short_attr;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(AsyncAttrDev, 1)

SCENARIO("Querying device with get_asynch_replies")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_asyn", "AsyncAttrDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AttrReadCallbackMockType callback;
        device->read_attribute_asynch("attr_asyn", callback);

        auto timeout = GENERATE(0, 500, -1);
        AND_GIVEN("we can read with timeout: " << timeout)
        {
            auto poll_func = get_poll_func(device, timeout);

            auto event = callback.pop_next_event(poll_func);

            AND_THEN("and get the attribute value")
            {
                using namespace Catch::Matchers;
                using namespace TangoTest::Matchers;

                REQUIRE(event != std::nullopt);

                REQUIRE_THAT(event,
                             EventValueMatches(AnyMatch(AnyLikeMatches(WithinAbs(ATTR_INIT_VALUE_DP, 0.0000001)))));

                std::vector<std::string> names{"attr_asyn"};
                REQUIRE_THAT(event->attr_names, RangeEquals(names));
            }
        }
    }
}

SCENARIO("Device timeout with get_asynch_replies")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_asyn", "AsyncAttrDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        // tweaking the device timeout allows us to execute the test faster
        device->set_timeout_millis(500);

        AttrReadCallbackMockType callback;
        device->read_attribute_asynch("attr_asyn_to", callback);

        auto timeout = GENERATE(0, 500, -1);
        AND_GIVEN("we can read with timeout: " << timeout)
        {
            auto poll_func = get_poll_func(device, timeout);

            auto event = callback.pop_next_event(poll_func);

            AND_THEN("get an error")
            {
                using namespace Catch::Matchers;
                using namespace TangoTest::Matchers;

                REQUIRE(event != std::nullopt);
                REQUIRE_THAT(event, EventErrorMatches(AnyMatch(Reason(Tango::API_DeviceTimedOut))));

                std::vector<std::string> names{"attr_asyn_to"};
                REQUIRE_THAT(event->attr_names, RangeEquals(names));
            }
        }
    }
}

SCENARIO("Device exception with get_asynch_replies")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_asyn", "AsyncAttrDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AttrReadCallbackMockType callback;
        device->read_attribute_asynch("attr_asyn_except", callback);

        auto timeout = GENERATE(0, 500, -1);
        AND_GIVEN("we can read with timeout: " << timeout)
        {
            auto poll_func = get_poll_func(device, timeout);

            auto event = callback.pop_next_event(poll_func);

            AND_THEN("get an error")
            {
                using namespace Catch::Matchers;
                using namespace TangoTest::Matchers;

                REQUIRE(event != std::nullopt);

                if(idlver >= 3)
                {
                    REQUIRE_THAT(event, EventErrorMatches(AllMatch(Reason(Tango::API_AttributeFailed))));
                    REQUIRE_THAT(event->argout, SizeIs(1));
                    REQUIRE_THAT(event->argout[0], ErrorListMatchesMatcher(AllMatch(Reason(TestExceptReason))));
                }
                else
                {
                    // we only have the event error list and no per attribute error list
                    // so we need to check with AnyMatch for both of them
                    REQUIRE_THAT(event, EventErrorMatches(AnyMatch(Reason(Tango::API_AttributeFailed))));
                    REQUIRE_THAT(event, EventErrorMatches(AnyMatch(Reason(TestExceptReason))));
                    REQUIRE_THAT(event->argout, IsEmpty());
                }

                std::vector<std::string> names{"attr_asyn_except"};
                REQUIRE_THAT(event->attr_names, RangeEquals(names));
            }
        }
    }
}

SCENARIO("multiple attributes can be read asynchronously")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_asyn", "AsyncAttrDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());
        AND_GIVEN("we read multiple attributes asynchronously")
        {
            std::vector<std::string> names{"attr_asyn", "short_attr"};

            AttrReadCallbackMockType callback;
            device->read_attributes_asynch(names, callback);

            AND_THEN("get a result back")
            {
                using namespace Catch::Matchers;
                using namespace TangoTest::Matchers;

                auto event = callback.pop_next_event([&]() { device->get_asynch_replies(); });
                REQUIRE(event != std::nullopt);
                REQUIRE(event->device != nullptr);
                REQUIRE_THAT(event->attr_names, RangeEquals(names));
                REQUIRE_THAT(event,
                             EventValueMatches(AnyMatch(AnyLikeMatches(WithinAbs(ATTR_INIT_VALUE_DP, 0.0000001)))));
                REQUIRE_THAT(event, EventValueMatches(AnyMatch(AnyLikeContains(ATTR_INIT_VALUE_SH))));
            }
        }
    }
}

SCENARIO("Error in readout callback reported")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    auto errorType = GENERATE(
        as<TangoTest::CallbackErrorType>{}, TangoTest::DevFailed, TangoTest::StdException, TangoTest::Arbitrary);

    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_asyn", "AsyncAttrDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AttrReadCallbackMockType callback;
        callback.set_error_in_callback(errorType);

        CaptureCerr cap;
        device->read_attribute_asynch("attr_asyn", callback);
        device->get_asynch_replies(500);
        check_callback_cerr_output(cap.str(), errorType);
    }
}

SCENARIO("Error in write callback reported")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    auto errorType = GENERATE(
        as<TangoTest::CallbackErrorType>{}, TangoTest::DevFailed, TangoTest::StdException, TangoTest::Arbitrary);

    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"attr_asyn", "AsyncAttrDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AttrWrittenEventCallbackMockType callback;
        callback.set_error_in_callback(errorType);

        Tango::DeviceAttribute send;

        send.set_name("attr_asyn");
        Tango::DevDouble lg = 22.2;
        send << lg;

        CaptureCerr cap;
        device->write_attribute_asynch(send, callback);
        device->get_asynch_replies(500);
        check_callback_cerr_output(cap.str(), errorType);
    }
}

SCENARIO("Error in cmd done callback reported")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    auto errorType = GENERATE(
        as<TangoTest::CallbackErrorType>{}, TangoTest::DevFailed, TangoTest::StdException, TangoTest::Arbitrary);

    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"cmd_asyn", "AsyncAttrDev", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        CmdDoneEventCallbackMockType callback;
        callback.set_error_in_callback(errorType);

        Tango::DevDouble lg = 22.2;
        Tango::DeviceData in;
        in << lg;

        CaptureCerr cap;
        device->command_inout_asynch("identity_double_cmd", in, callback);
        device->get_asynch_replies(500);
        check_callback_cerr_output(cap.str(), errorType);
    }
}
