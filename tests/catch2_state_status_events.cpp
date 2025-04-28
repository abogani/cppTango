#include "catch2_common.h"

template <class Base>
class EventDev : public Base
{
  public:
    using Base::Base;

    ~EventDev() override { }

    void init_device() override
    {
        Base::set_change_event("state", true, false);
    }

    void push_state_event()
    {
        Tango::Attribute &state_attr = Base::get_device_attr()->get_attr_by_name("state");

        for(int i = 0; i < 2; i++)
        {
            Tango::DevState *new_state = new Tango::DevState(Tango::ON);
            state_attr.set_value(new_state, 1, 0, true);
            state_attr.fire_change_event(nullptr);
        }
    }

    void push_status_event()
    {
        Tango::Attribute &status_attr = Base::get_device_attr()->get_attr_by_name("status");

        for(int i = 0; i < 2; ++i)
        {
            Tango::DevString *new_status = new Tango::DevString;
            *new_status = CORBA::string_dup("Status");
            status_attr.set_value(new_status, 1, 0, true);
            status_attr.fire_change_event(nullptr);
        }
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&EventDev::push_state_event>("push_state_event"));
        cmds.push_back(new TangoTest::AutoCommand<&EventDev::push_status_event>("push_status_event"));
    }
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(EventDev, 4)

SCENARIO("Generate change events for State or Status with some data")
{
    const auto [ctx_name, cmd_name] = GENERATE(table<std::string, std::string>(
        {{"change_event_state", "push_state_event"}, {"change_event_status", "push_status_event"}}));

    int idlver = GENERATE(TangoTest::idlversion(4));

    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{ctx_name, "EventDev", idlver};
        auto device = ctx.get_proxy();
        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("generate events")
        {
            REQUIRE_NOTHROW(device->command_inout(cmd_name));
        }
    }
}
