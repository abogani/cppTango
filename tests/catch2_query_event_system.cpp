#include "catch2_common.h"

#include <memory>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

template <typename Base>
class QueryESPub : public Base
{
  public:
    using Base::Base;

    ~QueryESPub() override { }

    void init_device() override
    {
        value = false;
    }

    void read_attr(Tango::Attribute &att) override
    {
        value = !value;
        att.set_value(&value);
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        using Attr = TangoTest::AutoAttr<&QueryESPub::read_attr>;

        attrs.push_back(new Attr("attr", Tango::DEV_BOOLEAN));
    }

  private:
    Tango::DevBoolean value;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(QueryESPub, 4)

template <typename Base>
class QueryESSub : public Base
{
  public:
    using Base::Base;

    ~QueryESSub() override { }

    void init_device() override { }

    struct Callback : public Tango::CallBack
    {
        void push_event(Tango::EventData *event) override
        {
            if(event == nullptr)
            {
                return;
            }

            if(event->err)
            {
                std::cout << "Got error event: " << event->errors;
            }
        }
    };

    void subscribe_to(Tango::DevString &attr_trl)
    {
        attr_proxy = std::make_unique<Tango::AttributeProxy>(attr_trl);
        attr_proxy->subscribe_event(Tango::EventType::CHANGE_EVENT, &callback);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&QueryESSub::subscribe_to>("SubscribeTo"));
    }

  private:
    Callback callback;
    std::unique_ptr<Tango::AttributeProxy> attr_proxy;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(QueryESSub, 4)

SCENARIO("QueryEventSystem works when there are no event subscriptions")
{
    GIVEN("a device proxy to an DServer device")
    {
        TangoTest::Context ctx{"query_es", "QueryESPub", 4};
        auto admin = ctx.get_admin_proxy();

        WHEN("We call QueryEventSystem")
        {
            Tango::DeviceData dd;
            REQUIRE_NOTHROW(dd = admin->command_inout("QueryEventSystem"));

            THEN("We get valid json")
            {
                using namespace Catch::Matchers;

                std::string str;
                dd >> str;

                INFO("QueryEventSystem returned: " << str);

                json obj;
                REQUIRE_NOTHROW(obj = json::parse(str));

                AND_THEN("There are no subscriptions reported")
                {
                    // At startup, the ZmqEventSupplier will have been created but
                    // the client won't have been.

                    REQUIRE(obj.contains("server"));
                    REQUIRE(obj["server"].is_object());
                    REQUIRE(obj["server"].contains("event_counters"));
                    REQUIRE(obj["server"]["event_counters"].is_object());
                    REQUIRE_THAT(obj["server"]["event_counters"], IsEmpty());

                    REQUIRE(obj.contains("client"));
                    REQUIRE(obj["client"].is_null());
                }
            }
        }
    }
}

SCENARIO("QueryEventSystem reports details about client side once subscribed")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a pair of IDLv" << idlver << " devices running in separate servers")
    {
        TangoTest::ContextDescriptor desc;

        desc.servers.push_back(TangoTest::ServerDescriptor{"query_es_sub", "QueryESSub", idlver});
        desc.servers.push_back(TangoTest::ServerDescriptor{"query_es_pub", "QueryESPub", idlver});

        TangoTest::Context ctx{desc};

        WHEN("We have one device subscribe to the other")
        {
            Tango::DeviceData dd;
            dd << ctx.get_fqtrl("query_es_pub", "attr");

            auto sub = ctx.get_proxy("query_es_sub");
            REQUIRE_NOTHROW(sub->command_inout("SubscribeTo", dd));

            THEN("The subscriber admin device reports the subscription")
            {
                using namespace Catch::Matchers;

                auto sub_admin = ctx.get_admin_proxy("query_es_sub");
                REQUIRE_NOTHROW(dd = sub_admin->command_inout("QueryEventSystem"));

                std::string str;
                dd >> str;

                INFO("Subscriber QueryEventSystem returned: " << str);

                json obj;
                REQUIRE_NOTHROW(obj = json::parse(str));

                REQUIRE(obj.contains("server"));
                REQUIRE(obj["server"].is_object());
                REQUIRE(obj["server"].contains("event_counters"));
                REQUIRE(obj["server"]["event_counters"].is_object());
                REQUIRE_THAT(obj["server"]["event_counters"], IsEmpty());

                REQUIRE(obj.contains("client"));
                REQUIRE(obj["client"].is_object());
                REQUIRE(obj["client"].contains("event_callbacks"));
                REQUIRE(obj["client"]["event_callbacks"].is_object());
                REQUIRE_THAT(obj["client"]["event_callbacks"], SizeIs(1));
                {
                    const auto &[key, val] = *obj["client"]["event_callbacks"].items().begin();
                    if(idlver >= 5)
                    {
                        REQUIRE_THAT(key, EndsWith("attr#dbase=no.idl5_change"));
                    }
                    else
                    {
                        REQUIRE_THAT(key, EndsWith("attr#dbase=no.change"));
                    }

                    REQUIRE(val.contains("channel_name"));
                    REQUIRE_THAT(val["channel_name"], ContainsSubstring("query_es_pub"));
                    REQUIRE(val.contains("callback_count"));
                    REQUIRE(val["callback_count"] == 1);
                    REQUIRE(val.contains("server_counter"));
                    REQUIRE(val["server_counter"] >= 0);
                    REQUIRE(val.contains("event_count"));
                    REQUIRE(val["event_count"] >= 0);
                    REQUIRE(val.contains("missed_event_count"));
                    REQUIRE(val["missed_event_count"] >= 0);
                    REQUIRE(val.contains("discarded_event_count"));
                    REQUIRE(val["discarded_event_count"] >= 0);
                    REQUIRE(val.contains("last_resubscribed"));
                    REQUIRE(val["last_resubscribed"].is_null());
                }

                REQUIRE(obj["client"].contains("event_channels"));
                REQUIRE(obj["client"]["event_channels"].is_object());
                REQUIRE_THAT(obj["client"]["event_channels"], SizeIs(1));
                {
                    const auto &[key, val] = *obj["client"]["event_channels"].items().begin();
                    REQUIRE_THAT(key, EndsWith("query_es_pub#dbase=no"));
                    REQUIRE(val.contains("endpoint"));
                }
            }

            THEN("The publisher admin device reports the subscription")
            {
                using namespace Catch::Matchers;

                auto pub_admin = ctx.get_admin_proxy("query_es_pub");
                REQUIRE_NOTHROW(dd = pub_admin->command_inout("QueryEventSystem"));

                std::string str;
                dd >> str;

                INFO("Publisher QueryEventSystem returned: " << str);

                json obj;
                REQUIRE_NOTHROW(obj = json::parse(str));

                REQUIRE(obj.contains("server"));
                REQUIRE(obj["server"].is_object());
                REQUIRE(obj["server"].contains("event_counters"));
                REQUIRE(obj["server"]["event_counters"].is_object());
                REQUIRE_THAT(obj["server"]["event_counters"], SizeIs(1));
                const auto &[key, val] = *obj["server"]["event_counters"].items().begin();
                REQUIRE_THAT(key, EndsWith("attr#dbase=no.change"));
                REQUIRE(val > 0);

                REQUIRE(obj.contains("client"));
                REQUIRE(obj["client"].is_null());
            }
        }
    }
}
