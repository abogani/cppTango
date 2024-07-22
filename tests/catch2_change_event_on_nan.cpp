#include <tango/tango.h>
#include <memory>
#include <cmath>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "catch2_common.h"

constexpr static Tango::DevDouble ATTR_INIT_VALUE = 0.0;
static const Tango::DevDouble ATTR_NAN_VALUE = std::nan("nan");

// Test device class for Tango::DevDouble
template <class Base>
class ChangeEventOnNanDev_Double : public Base
{
  public:
    using Base::Base;

    ~ChangeEventOnNanDev_Double() override { }

    void init_device() override
    {
        attr_abs_value = ATTR_INIT_VALUE;
        attr_rel_value = ATTR_INIT_VALUE;
    }

    void set_abs_nan()
    {
        attr_abs_value = ATTR_NAN_VALUE;
    }

    void set_rel_nan()
    {
        attr_rel_value = ATTR_NAN_VALUE;
    }

    void unset_abs_nan()
    {
        attr_abs_value = ATTR_INIT_VALUE;
    }

    void unset_rel_nan()
    {
        attr_rel_value = ATTR_INIT_VALUE;
    }

    void read_abs(Tango::Attribute &att)
    {
        att.set_value_date_quality(&attr_abs_value, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    void read_rel(Tango::Attribute &att)
    {
        att.set_value_date_quality(&attr_rel_value, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        // attribute with absolute change
        auto attr_abs = new TangoTest::AutoAttr<&ChangeEventOnNanDev_Double::read_abs>("attr_abs", Tango::DEV_DOUBLE);
        Tango::UserDefaultAttrProp abs_props;
        abs_props.set_event_abs_change("0.01");
        attr_abs->set_default_properties(abs_props);
        attr_abs->set_polling_period(100);
        attrs.push_back(attr_abs);

        // attribute with relative change
        auto attr_rel = new TangoTest::AutoAttr<&ChangeEventOnNanDev_Double::read_rel>("attr_rel", Tango::DEV_DOUBLE);
        Tango::UserDefaultAttrProp rel_props;
        rel_props.set_event_rel_change("0.01");
        attr_rel->set_default_properties(rel_props);
        attr_rel->set_polling_period(100);
        attrs.push_back(attr_rel);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev_Double::set_abs_nan>("set_abs_nan"));
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev_Double::unset_abs_nan>("unset_abs_nan"));
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev_Double::set_rel_nan>("set_rel_nan"));
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev_Double::unset_rel_nan>("unset_rel_nan"));
    }

  private:
    Tango::DevDouble attr_abs_value;
    Tango::DevDouble attr_rel_value;
};

// Test device class for Tango::DevFloat
template <class Base>
class ChangeEventOnNanDev_Float : public Base
{
  public:
    using Base::Base;

    ~ChangeEventOnNanDev_Float() override { }

    void init_device() override
    {
        attr_abs_value = ATTR_INIT_VALUE;
        attr_rel_value = ATTR_INIT_VALUE;
    }

    void set_abs_nan()
    {
        attr_abs_value = ATTR_NAN_VALUE;
    }

    void set_rel_nan()
    {
        attr_rel_value = ATTR_NAN_VALUE;
    }

    void unset_abs_nan()
    {
        attr_abs_value = ATTR_INIT_VALUE;
    }

    void unset_rel_nan()
    {
        attr_rel_value = ATTR_INIT_VALUE;
    }

    void read_abs(Tango::Attribute &att)
    {
        att.set_value_date_quality(&attr_abs_value, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    void read_rel(Tango::Attribute &att)
    {
        att.set_value_date_quality(&attr_rel_value, std::chrono::system_clock::now(), Tango::ATTR_VALID);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        // attribute with absolute change
        auto attr_abs = new TangoTest::AutoAttr<&ChangeEventOnNanDev_Float::read_abs>("attr_abs", Tango::DEV_FLOAT);
        Tango::UserDefaultAttrProp abs_props;
        abs_props.set_event_abs_change("0.01");
        attr_abs->set_default_properties(abs_props);
        attr_abs->set_polling_period(100);
        attrs.push_back(attr_abs);

        // attribute with relative change
        auto attr_rel = new TangoTest::AutoAttr<&ChangeEventOnNanDev_Float::read_rel>("attr_rel", Tango::DEV_FLOAT);
        Tango::UserDefaultAttrProp rel_props;
        rel_props.set_event_rel_change("0.01");
        attr_rel->set_default_properties(rel_props);
        attr_rel->set_polling_period(100);
        attrs.push_back(attr_rel);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev_Float::set_abs_nan>("set_abs_nan"));
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev_Float::unset_abs_nan>("unset_abs_nan"));
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev_Float::set_rel_nan>("set_rel_nan"));
        cmds.push_back(new TangoTest::AutoCommand<&ChangeEventOnNanDev_Float::unset_rel_nan>("unset_rel_nan"));
    }

  private:
    Tango::DevFloat attr_abs_value;
    Tango::DevFloat attr_rel_value;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(ChangeEventOnNanDev_Double, 4)

SCENARIO("Change events for DevDouble are generated on NaN with absolute change")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"change_event_on_nan", "ChangeEventOnNanDev_Double", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute with absolute change")
        {
            std::string att{"attr_abs"};

            REQUIRE(device->is_attribute_polled(att));

            AND_GIVEN("a change event subscription")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::CHANGE_EVENT, &callback));

                THEN("we receive some events with the initial value")
                {
                    using Catch::Matchers::IsNaN;
                    using Catch::Matchers::WithinAbs;
                    using TangoTest::AnyLikeMatches;
                    // We get the following two initial events (the fact there
                    // are two is a side effect of the fix for #369):
                    //
                    // 1. In `subscribe_event` we do a `read_attribute` to
                    // generate the first event
                    // 2. Because we are the first subscriber to `"attr"`, the
                    // polling loop starts and sends an event because it is the
                    // first time it has read the attribute

                    auto maybe_initial_event = callback.pop_next_event();
                    REQUIRE(maybe_initial_event.has_value());
                    REQUIRE(!maybe_initial_event->err);
                    REQUIRE(maybe_initial_event->attr_value != nullptr);
                    REQUIRE_THAT(*maybe_initial_event->attr_value,
                                 AnyLikeMatches(WithinAbs(ATTR_INIT_VALUE, 0.0000001)));

                    maybe_initial_event = callback.pop_next_event();

                    WHEN("we set the attribute value to NaN")
                    {
                        REQUIRE_NOTHROW(device->command_inout("set_abs_nan"));

                        THEN("a change event is generated")
                        {
                            auto maybe_new_event = callback.pop_next_event();
                            REQUIRE(maybe_new_event.has_value());
                            REQUIRE(!maybe_new_event->err);
                            REQUIRE(maybe_new_event->attr_value != nullptr);
                            REQUIRE_THAT(*maybe_new_event->attr_value, AnyLikeMatches(IsNaN()));
                            AND_WHEN("we unset the attribute value from NaN")
                            {
                                REQUIRE_NOTHROW(device->command_inout("unset_abs_nan"));

                                THEN("a change event is generated")
                                {
                                    auto maybe_new_event = callback.pop_next_event();
                                    REQUIRE(maybe_new_event.has_value());
                                    REQUIRE(!maybe_new_event->err);
                                    REQUIRE(maybe_new_event->attr_value != nullptr);
                                    REQUIRE_THAT(*maybe_new_event->attr_value,
                                                 AnyLikeMatches(WithinAbs(ATTR_INIT_VALUE, 0.0000001)));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Change events for DevDouble are generated on NaN with relative change")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"change_event_on_nan", "ChangeEventOnNanDev_Double", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute with relative change")
        {
            std::string att{"attr_rel"};

            REQUIRE(device->is_attribute_polled(att));

            AND_GIVEN("a change event subscription")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::CHANGE_EVENT, &callback));
                THEN("we receive some events with the initial value")
                {
                    using Catch::Matchers::IsNaN;
                    using Catch::Matchers::WithinAbs;
                    using TangoTest::AnyLikeMatches;

                    // We get the following two initial events (the fact there
                    // are two is a side effect of the fix for #369):
                    //
                    // 1. In `subscribe_event` we do a `read_attribute` to
                    // generate the first event
                    // 2. Because we are the first subscriber to `"attr"`, the
                    // polling loop starts and sends an event because it is the
                    // first time it has read the attribute

                    auto maybe_initial_event = callback.pop_next_event();
                    REQUIRE(maybe_initial_event.has_value());
                    REQUIRE(!maybe_initial_event->err);
                    REQUIRE(maybe_initial_event->attr_value != nullptr);
                    REQUIRE_THAT(*maybe_initial_event->attr_value,
                                 AnyLikeMatches(WithinAbs(ATTR_INIT_VALUE, 0.0000001)));

                    maybe_initial_event = callback.pop_next_event();

                    WHEN("we set the attribute value to NaN")
                    {
                        REQUIRE_NOTHROW(device->command_inout("set_rel_nan"));
                        THEN("a change event is generated")
                        {
                            auto maybe_new_event = callback.pop_next_event();
                            REQUIRE(maybe_new_event.has_value());
                            REQUIRE(!maybe_new_event->err);
                            REQUIRE(maybe_new_event->attr_value != nullptr);
                            REQUIRE_THAT(*maybe_new_event->attr_value, AnyLikeMatches(IsNaN()));

                            AND_WHEN("we unset the attribute value from NaN")
                            {
                                REQUIRE_NOTHROW(device->command_inout("unset_rel_nan"));
                                THEN("a change event is generated")
                                {
                                    auto maybe_new_event = callback.pop_next_event();
                                    REQUIRE(maybe_new_event.has_value());
                                    REQUIRE(!maybe_new_event->err);
                                    REQUIRE(maybe_new_event->attr_value != nullptr);
                                    REQUIRE_THAT(*maybe_new_event->attr_value,
                                                 AnyLikeMatches(WithinAbs(ATTR_INIT_VALUE, 0.0000001)));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(ChangeEventOnNanDev_Float, 4)

SCENARIO("Change events for DevFloat are generated on NaN with absolute change")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"change_event_on_nan", "ChangeEventOnNanDev_Float", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute with absolute change")
        {
            std::string att{"attr_abs"};

            REQUIRE(device->is_attribute_polled(att));

            AND_GIVEN("a change event subscription")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::CHANGE_EVENT, &callback));

                THEN("we receive some events with the initial value")
                {
                    using Catch::Matchers::IsNaN;
                    using Catch::Matchers::WithinAbs;
                    using TangoTest::AnyLikeMatches;

                    // We get the following two initial events (the fact there
                    // are two is a side effect of the fix for #369):
                    //
                    // 1. In `subscribe_event` we do a `read_attribute` to
                    // generate the first event
                    // 2. Because we are the first subscriber to `"attr"`, the
                    // polling loop starts and sends an event because it is the
                    // first time it has read the attribute

                    auto maybe_initial_event = callback.pop_next_event();
                    REQUIRE(maybe_initial_event.has_value());
                    REQUIRE(!maybe_initial_event->err);
                    REQUIRE(maybe_initial_event->attr_value != nullptr);
                    REQUIRE_THAT(*maybe_initial_event->attr_value,
                                 AnyLikeMatches<float>(WithinAbs(ATTR_INIT_VALUE, 0.0000001f)));

                    maybe_initial_event = callback.pop_next_event();

                    WHEN("we set the attribute value to NaN")
                    {
                        REQUIRE_NOTHROW(device->command_inout("set_abs_nan"));

                        THEN("a change event is generated")
                        {
                            auto maybe_new_event = callback.pop_next_event();
                            REQUIRE(maybe_new_event.has_value());
                            REQUIRE(!maybe_new_event->err);
                            REQUIRE(maybe_new_event->attr_value != nullptr);
                            REQUIRE_THAT(*maybe_new_event->attr_value, AnyLikeMatches<float>(IsNaN()));
                            AND_WHEN("we unset the attribute value from NaN")
                            {
                                REQUIRE_NOTHROW(device->command_inout("unset_abs_nan"));

                                THEN("a change event is generated")
                                {
                                    auto maybe_new_event = callback.pop_next_event();
                                    REQUIRE(maybe_new_event.has_value());
                                    REQUIRE(!maybe_new_event->err);
                                    REQUIRE(maybe_new_event->attr_value != nullptr);
                                    REQUIRE_THAT(*maybe_new_event->attr_value,
                                                 AnyLikeMatches<float>(WithinAbs(ATTR_INIT_VALUE, 0.0000001f)));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Change events for DevFloat are generated on NaN with relative change")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a device proxy to a simple IDLv" << idlver << " device")
    {
        TangoTest::Context ctx{"change_event_on_nan", "ChangeEventOnNanDev_Float", idlver};
        auto device = ctx.get_proxy();

        REQUIRE(idlver == device->get_idl_version());

        AND_GIVEN("a polled attribute with relative change")
        {
            std::string att{"attr_rel"};

            REQUIRE(device->is_attribute_polled(att));

            AND_GIVEN("a change event subscription")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;
                REQUIRE_NOTHROW(device->subscribe_event(att, Tango::CHANGE_EVENT, &callback));
                THEN("we receive some events with the initial value")
                {
                    using Catch::Matchers::IsNaN;
                    using Catch::Matchers::WithinAbs;
                    using TangoTest::AnyLikeMatches;

                    // We get the following two initial events (the fact there
                    // are two is a side effect of the fix for #369):
                    //
                    // 1. In `subscribe_event` we do a `read_attribute` to
                    // generate the first event
                    // 2. Because we are the first subscriber to `"attr"`, the
                    // polling loop starts and sends an event because it is the
                    // first time it has read the attribute

                    auto maybe_initial_event = callback.pop_next_event();
                    REQUIRE(maybe_initial_event.has_value());
                    REQUIRE(!maybe_initial_event->err);
                    REQUIRE(maybe_initial_event->attr_value != nullptr);
                    REQUIRE_THAT(*maybe_initial_event->attr_value,
                                 AnyLikeMatches<float>(WithinAbs(ATTR_INIT_VALUE, 0.0000001f)));

                    maybe_initial_event = callback.pop_next_event();

                    WHEN("we set the attribute value to NaN")
                    {
                        REQUIRE_NOTHROW(device->command_inout("set_rel_nan"));
                        THEN("a change event is generated")
                        {
                            auto maybe_new_event = callback.pop_next_event();
                            REQUIRE(maybe_new_event.has_value());
                            REQUIRE(!maybe_new_event->err);
                            REQUIRE(maybe_new_event->attr_value != nullptr);
                            REQUIRE_THAT(*maybe_new_event->attr_value, AnyLikeMatches<float>(IsNaN()));

                            AND_WHEN("we unset the attribute value from NaN")
                            {
                                REQUIRE_NOTHROW(device->command_inout("unset_rel_nan"));
                                THEN("a change event is generated")
                                {
                                    auto maybe_new_event = callback.pop_next_event();
                                    REQUIRE(maybe_new_event.has_value());
                                    REQUIRE(!maybe_new_event->err);
                                    REQUIRE(maybe_new_event->attr_value != nullptr);
                                    REQUIRE_THAT(*maybe_new_event->attr_value,
                                                 AnyLikeMatches<float>(WithinAbs(ATTR_INIT_VALUE, 0.0000001f)));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
