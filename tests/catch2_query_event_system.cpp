#include "catch2_common.h"

#include <memory>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

static constexpr const Tango::DevLong k_event_count = 3;

template <typename Base>
class QueryESPub : public Base
{
  public:
    using Base::Base;

    ~QueryESPub() override { }

    void init_device() override
    {
        value = 0;
        this->set_change_event("attr", true, false);
    }

    void read_attr(Tango::Attribute &att) override
    {
        att.set_value(&value);
    }

    void push_events()
    {
        for(Tango::DevLong i = 0; i < k_event_count; ++i)
        {
            value += 1;
            this->push_change_event("attr", &value);
        }
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        using Attr = TangoTest::AutoAttr<&QueryESPub::read_attr>;

        attrs.push_back(new Attr("attr", Tango::DEV_LONG));
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&QueryESPub::push_events>("PushEvents"));
    }

  private:
    Tango::DevLong value;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(QueryESPub, 4)

template <typename Base>
class QueryESSub : public Base
{
  public:
    using Base::Base;

    ~QueryESSub() override { }

    void init_device() override
    {
        callback.device = this;

        // This device will send one event on this attribute, once it has
        // received 4 events from whoever it has subscribed to
        this->set_change_event("received", true, false);
    }

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
                std::cout << "Got error event: " << event->errors << "\n";
            }
            else
            {
                if(event->attr_value != nullptr)
                {
                    Tango::DevLong value;
                    *event->attr_value >> value;
                    std::cout << "Got event: " << value << "\n";
                }
                else
                {
                    std::cout << "Got event with no value or error\n";
                }

                count++;
                // +1 to include the initial event generated during subscribe_event
                if(count >= k_event_count + 1)
                {
                    device->push_change_event("received", &count);
                }
            }
        }

        QueryESSub *device;
        Tango::DevLong count = 0;
    };

    void read_attr(Tango::Attribute &att) override
    {
        att.set_value(&callback.count);
    }

    void subscribe_to(Tango::DevString &attr_trl)
    {
        attr_proxy = std::make_unique<Tango::AttributeProxy>(attr_trl);
        attr_proxy->subscribe_event(Tango::EventType::CHANGE_EVENT, &callback);
    }

    static void command_factory(std::vector<Tango::Command *> &cmds)
    {
        cmds.push_back(new TangoTest::AutoCommand<&QueryESSub::subscribe_to>("SubscribeTo"));
    }

    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
    {
        using Attr = TangoTest::AutoAttr<&QueryESSub::read_attr>;

        attrs.push_back(new Attr("received", Tango::DEV_LONG));
    }

  private:
    Callback callback;
    std::unique_ptr<Tango::AttributeProxy> attr_proxy;
};

TANGO_TEST_AUTO_DEV_TMPL_INSTANTIATE(QueryESSub, 4)

namespace
{
void require_server_sample(const json &obj)
{
    REQUIRE(obj.is_object());
    REQUIRE(obj.contains("micros_since_last_event"));
    REQUIRE(obj.contains("push_event_micros"));
}

void require_client_sample(const json &obj)
{
    REQUIRE(obj.is_object());
    REQUIRE(obj.contains("discarded"));
    REQUIRE(obj.contains("attr_name"));
    REQUIRE(obj.contains("micros_since_last_event"));
    REQUIRE(obj.contains("sleep_micros"));
    REQUIRE(obj.contains("process_micros"));
    REQUIRE(obj.contains("first_callback_latency_micros"));
    REQUIRE(obj.contains("callback_count"));
    REQUIRE(obj.contains("wake_count"));
}

void require_event_callback_value(const json &obj)
{
    REQUIRE(obj.is_object());
    REQUIRE(obj.contains("channel_name"));
    REQUIRE(obj.contains("callback_count"));
    REQUIRE(obj.contains("server_counter"));
    REQUIRE(obj.contains("event_count"));
    REQUIRE(obj.contains("missed_event_count"));
    REQUIRE(obj.contains("discarded_event_count"));
    REQUIRE(obj.contains("last_resubscribed"));
}

void require_client_object(const json &obj)
{
    REQUIRE(obj.is_object());
    REQUIRE(obj.contains("event_callbacks"));
    REQUIRE(obj.contains("event_channels"));
    REQUIRE(obj.contains("perf"));
}

void require_server_object(const json &obj)
{
    REQUIRE(obj.is_object());
    REQUIRE(obj.contains("event_counters"));
    REQUIRE(obj.contains("perf"));
}

void require_query_object(const json &obj)
{
    REQUIRE(obj.is_object());
    REQUIRE(obj.contains("server"));
    REQUIRE(obj.contains("client"));
}

const json &require_server_and_null_client(const json &obj)
{
    require_query_object(obj);
    require_server_object(obj["server"]);
    REQUIRE(obj["client"].is_null());

    return obj["server"];
}

std::pair<const json &, const json &> require_server_and_client(const json &obj)
{
    require_query_object(obj);
    require_server_object(obj["server"]);
    require_client_object(obj["client"]);

    return {obj["server"], obj["client"]};
}
} // namespace

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

                    const auto &server = require_server_and_null_client(obj);
                    REQUIRE(server["event_counters"].is_object());
                    REQUIRE_THAT(server["event_counters"], IsEmpty());
                    REQUIRE(server["perf"].is_null());
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
                const auto &[server, client] = require_server_and_client(obj);

                REQUIRE(server["perf"].is_null());
                REQUIRE(server["event_counters"].is_object());
                REQUIRE_THAT(obj["server"]["event_counters"], IsEmpty());

                REQUIRE(client["perf"].is_null());

                REQUIRE(client["event_callbacks"].is_object());
                REQUIRE_THAT(client["event_callbacks"], SizeIs(1));
                for(const auto &[key, val] : client["event_callbacks"].items())
                {
                    if(idlver >= 5)
                    {
                        REQUIRE_THAT(key, EndsWith("attr#dbase=no.idl5_change"));
                    }
                    else
                    {
                        REQUIRE_THAT(key, EndsWith("attr#dbase=no.change"));
                    }

                    require_event_callback_value(val);

                    CHECK_THAT(val["channel_name"], ContainsSubstring("query_es_pub"));
                    CHECK(val["callback_count"] == 1);
                    CHECK(val["server_counter"] == 0);
                    CHECK(val["event_count"] == 0);
                    CHECK(val["missed_event_count"] == 0);
                    CHECK(val["discarded_event_count"] == 0);
                    CHECK(val["last_resubscribed"].is_null());
                }

                REQUIRE_THAT(client["event_channels"], SizeIs(1));
                for(const auto &[key, val] : client["event_channels"].items())
                {
                    CHECK_THAT(key, EndsWith("query_es_pub#dbase=no"));
                    CHECK(val.contains("endpoint"));
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
                const auto &server = require_server_and_null_client(obj);

                CHECK(server["perf"].is_null());
                CHECK(server["event_counters"].is_object());
                CHECK_THAT(server["event_counters"], SizeIs(1));
                for(const auto &[key, val] : server["event_counters"].items())
                {
                    CHECK_THAT(key, EndsWith("attr#dbase=no.change"));
                    CHECK(val > 0);
                }
            }
        }
    }
}

SCENARIO("QueryEventSystem can report performance samples")
{
    int idlver = GENERATE(TangoTest::idlversion(4));
    GIVEN("a pair of IDLv" << idlver << " devices running in separate servers")
    {
        TangoTest::ContextDescriptor desc;

        desc.servers.push_back(TangoTest::ServerDescriptor{"query_es_sub", "QueryESSub", idlver});
        desc.servers.push_back(TangoTest::ServerDescriptor{"query_es_pub", "QueryESPub", idlver});

        TangoTest::Context ctx{desc};

        WHEN("We enable performance monitoring")
        {
            Tango::DeviceData dd;

            dd << true;
            auto sub_admin = ctx.get_admin_proxy("query_es_sub");
            REQUIRE_NOTHROW(sub_admin->command_inout("EnableEventSystemPerfMon", dd));

            dd << true;
            auto pub_admin = ctx.get_admin_proxy("query_es_pub");
            REQUIRE_NOTHROW(pub_admin->command_inout("EnableEventSystemPerfMon", dd));

            AND_WHEN("We have one device subscribe to the other")
            {
                TangoTest::CallbackMock<Tango::EventData> callback;

                auto sub = ctx.get_proxy("query_es_sub");
                sub->subscribe_event("received", Tango::CHANGE_EVENT, &callback);
                require_initial_events(callback);

                dd << ctx.get_fqtrl("query_es_pub", "attr");
                REQUIRE_NOTHROW(sub->command_inout("SubscribeTo", dd));

                auto pub = ctx.get_proxy("query_es_pub");
                REQUIRE_NOTHROW(pub->command_inout("PushEvents"));

                REQUIRE(callback.pop_next_event() != std::nullopt);

                THEN("The subscriber admin device reports performance data")
                {
                    using namespace Catch::Matchers;

                    REQUIRE_NOTHROW(dd = sub_admin->command_inout("QueryEventSystem"));

                    std::string str;
                    dd >> str;

                    INFO("Subscriber QueryEventSystem returned: " << str);

                    json obj;
                    REQUIRE_NOTHROW(obj = json::parse(str));
                    const auto &[server, client] = require_server_and_client(obj);

                    REQUIRE(server["perf"].is_array());
                    REQUIRE_THAT(server["perf"], SizeIs(1));

                    // On my system, I always get one discarded event, but I
                    // don't want to assume that we always get one for this
                    // test.
                    size_t discarded_event_count = 0;
                    REQUIRE(client["event_callbacks"].is_object());
                    REQUIRE_THAT(client["event_callbacks"], SizeIs(1));
                    for(const auto &[key, val] : obj["client"]["event_callbacks"].items())
                    {
                        if(idlver >= 5)
                        {
                            REQUIRE_THAT(key, EndsWith("attr#dbase=no.idl5_change"));
                        }
                        else
                        {
                            REQUIRE_THAT(key, EndsWith("attr#dbase=no.change"));
                        }

                        require_event_callback_value(val);

                        CHECK_THAT(val["channel_name"], ContainsSubstring("query_es_pub"));
                        CHECK(val["callback_count"] == 1);
                        CHECK(val["server_counter"] == k_event_count);
                        CHECK(val["discarded_event_count"] >= 0);
                        discarded_event_count = val["discarded_event_count"];
                        CHECK(val["event_count"] == k_event_count);
                        CHECK(val["missed_event_count"] == 0);
                        CHECK(val["last_resubscribed"].is_null());
                    }

                    REQUIRE(client["perf"].is_array());
                    REQUIRE_THAT(client["perf"], SizeIs(k_event_count + discarded_event_count));

                    bool first = true;
                    for(const auto &sample : client["perf"])
                    {
                        require_client_sample(sample);

                        REQUIRE(sample["discarded"].is_boolean());
                        if(!sample["discarded"])
                        {
                            CHECK(sample["attr_name"] == "attr#dbase=no");
                            if(first)
                            {
                                CHECK(sample["micros_since_last_event"].is_null());
                            }
                            else
                            {
                                CHECK(sample["micros_since_last_event"].is_number());
                            }
                            CHECK(sample["sleep_micros"].is_number());
                            CHECK(sample["process_micros"].is_number());
                            CHECK(sample["first_callback_latency_micros"].is_number());
                            CHECK(sample["callback_count"] == 1);
                            CHECK(sample["wake_count"] >= 1);
                        }
                        else
                        {
                            CHECK(sample["attr_name"] == "");
                            if(first)
                            {
                                CHECK(sample["micros_since_last_event"].is_null());
                            }
                            else
                            {
                                CHECK(sample["micros_since_last_event"].is_number());
                            }
                            CHECK(sample["sleep_micros"].is_number());
                            CHECK(sample["process_micros"].is_number());
                            CHECK(sample["first_callback_latency_micros"].is_null());
                            CHECK(sample["callback_count"] == 0);
                            CHECK(sample["wake_count"] >= 1);
                        }

                        first = false;
                    }

                    AND_WHEN("we query again")
                    {
                        REQUIRE_NOTHROW(dd = sub_admin->command_inout("QueryEventSystem"));

                        dd >> str;

                        INFO("Subscriber second QueryEventSystem returned: " << str);

                        THEN("there is no performance data")
                        {
                            REQUIRE_NOTHROW(obj = json::parse(str));
                            const auto &[server, client] = require_server_and_client(obj);

                            REQUIRE(server["perf"].is_array());
                            REQUIRE_THAT(server["perf"], IsEmpty());

                            REQUIRE(client["perf"].is_array());
                            REQUIRE_THAT(client["perf"], IsEmpty());
                        }
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
                    const auto &server = require_server_and_null_client(obj);

                    REQUIRE(server["perf"].is_array());
                    REQUIRE_THAT(server["perf"], SizeIs(k_event_count));

                    bool first = true;
                    for(const auto &sample : server["perf"])
                    {
                        require_server_sample(sample);

                        if(first)
                        {
                            CHECK(sample["micros_since_last_event"].is_null());
                        }
                        else
                        {
                            CHECK(sample["micros_since_last_event"].is_number());
                        }
                        CHECK(sample["push_event_micros"].is_number());

                        first = false;
                    }

                    AND_WHEN("we query again")
                    {
                        REQUIRE_NOTHROW(dd = pub_admin->command_inout("QueryEventSystem"));

                        dd >> str;

                        INFO("Publisher second QueryEventSystem returned: " << str);

                        THEN("there is no performance data")
                        {
                            REQUIRE_NOTHROW(obj = json::parse(str));
                            const auto &server = require_server_and_null_client(obj);

                            CHECK(server["perf"].is_array());
                            CHECK_THAT(server["perf"], IsEmpty());
                        }
                    }
                }
            }
        }
    }
}
