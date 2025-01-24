#include "catch2_common.h"

#include <memory>
#include <chrono>
#include <thread>

using namespace std::literals::chrono_literals;

template <class Base>
class ConnectionTest : public Base
{
  public:
    using Base::Base;

    ~ConnectionTest() override { }

    void init_device() override
    {
        if constexpr(std::is_base_of_v<Tango::Device_6Impl, Base>)
        {
            this->add_version_info("ConnectionTest", "1.0.0");
        }
    }

    Tango::DevLong next()
    {
        Tango::DevLong result = m_counter;
        m_counter++;
        return result;
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&ConnectionTest::next>("next"));
    }

  private:
    Tango::DevLong m_counter = 0;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(ConnectionTest, 1)

// TODO: Extend this test to other IDL versions.  For each other IDL version, V:
// - Work out things which you can only do with that IDL version, i.e. those
//   things which require using `Connection::device_V`.
// - Add stuff to the `ConnectionTest` class inside
//   `if constexpr(std::is_base_of_v<Tango::Device_VImpl, Base>)` blocks
// - Add client side tests with the assignee/copy inside an `if (idlver >= V)` block

SCENARIO("DeviceProxy objects can be copied and assigned")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        using namespace TangoTest::Matchers;

        TangoTest::Context ctx{"connection_test", "ConnectionTest", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        {
            Tango::DeviceData dd = device->command_inout("next");
            REQUIRE_THAT(dd, AnyLikeContains(static_cast<Tango::DevLong>(0)));
        }

        WHEN("we copy the device proxy")
        {
            auto copy = std::make_unique<Tango::DeviceProxy>(*device);

            THEN("The copy has the same name as the original")
            {
                REQUIRE(device->dev_name() == copy->dev_name());
            }

            if(idlver >= 6)
            {
                THEN("we can query the device version list with the copy")
                {
                    Tango::DeviceInfo di;
                    REQUIRE_NOTHROW(di = copy->info());
                    REQUIRE(di.version_info.find("ConnectionTest") != di.version_info.end());
                }
            }

            THEN("we can invoke a command on the same object with the copy")
            {
                Tango::DeviceData dd = copy->command_inout("next");
                REQUIRE_THAT(dd, AnyLikeContains(static_cast<Tango::DevLong>(1)));
            }
        }

        WHEN("we assign the device proxy")
        {
            auto assignee = std::make_unique<Tango::DeviceProxy>();
            *assignee = *device;

            THEN("The assignee has the same name as the original")
            {
                REQUIRE(device->dev_name() == assignee->dev_name());
            }

            if(idlver >= 6)
            {
                THEN("we can query the device version list with the assignee")
                {
                    Tango::DeviceInfo di;
                    REQUIRE_NOTHROW(di = assignee->info());
                    REQUIRE(di.version_info.find("ConnectionTest") != di.version_info.end());
                }
            }

            THEN("we can invoke a command on the same object with the assignee")
            {
                Tango::DeviceData dd = assignee->command_inout("next");
                REQUIRE_THAT(dd, AnyLikeContains(static_cast<Tango::DevLong>(1)));
            }
        }
    }
}

template <class Base>
class TimeoutAttrRead : public Base
{
  public:
    using Base::Base;

    ~TimeoutAttrRead() override { }

    void init_device() override { }

    void read_attr(Tango::Attribute &att) override
    {
        std::this_thread::sleep_for(500ms);
        att.set_value(&m_value);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        attrs.push_back(new TangoTest::AutoAttr<&TimeoutAttrRead::read_attr>("slow_attr", Tango::DEV_LONG));
    }

  private:
    Tango::DevLong m_value = 0;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(TimeoutAttrRead, 1)

SCENARIO("DeviceProxy objects can have the timeout set")
{
    int idlver = GENERATE(TangoTest::idlversion(1));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        using namespace TangoTest::Matchers;

        TangoTest::Context ctx{"connection_test", "TimeoutAttrRead", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        WHEN("we set a short timeout for the device proxy")
        {
            device->set_timeout_millis(100);

            THEN("a slow attribute read times out as expected")
            {
                Tango::DeviceAttribute result;
                REQUIRE_THROWS_MATCHES(result = device->read_attribute("slow_attr"),
                                       Tango::DevFailed,
                                       ErrorListMatches(AnyMatch(Reason(Tango::API_DeviceTimedOut))));
            }
        }
    }
}
