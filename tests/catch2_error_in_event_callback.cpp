#include "catch2_common.h"

template <class Base>
class CallbackErrorDev : public Base
{
  public:
    using Base::Base;

    ~CallbackErrorDev() override { }

    void init_device() override { }

    void read_attr(Tango::Attribute &att) override
    {
        if(att.get_name() == "double_attr")
        {
            att.set_value(&attr_val);
        }
    }

    void push_event()
    {
        Tango::Attribute &double_attr = Base::get_device_attr()->get_attr_by_name("double_attr");
        double_attr.set_value(&attr_val);
        double_attr.fire_change_event(nullptr);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        auto attr = new TangoTest::AutoAttr<&CallbackErrorDev::read_attr>("double_attr", Tango::DEV_DOUBLE);
        attr->set_change_event(true, false);
        attrs.push_back(attr);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&CallbackErrorDev::push_event>("push_event"));
    }

    Tango::DevDouble attr_val = 33.3;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(CallbackErrorDev, 4)

SCENARIO("Error in event callback reported")
{
    int idlver = GENERATE(TangoTest::idlversion(4));

    auto errorType = GENERATE(
        as<TangoTest::CallbackErrorType>{}, TangoTest::DevFailed, TangoTest::StdException, TangoTest::Arbitrary);

    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"error_in_event_callback", "CallbackErrorDev", idlver};
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        TangoTest::CallbackMock<Tango::EventData> callback;
        REQUIRE_NOTHROW(device->subscribe_event("double_attr", Tango::CHANGE_EVENT, &callback));

        callback.set_error_in_callback(errorType);
        CaptureCerr cap;
        REQUIRE_NOTHROW(device->command_inout("push_event"));
        std::this_thread::sleep_for(std::chrono::seconds(1));
        check_callback_cerr_output(cap.str(), errorType);
    }
}
