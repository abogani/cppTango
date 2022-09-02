//
// Created by pxh on 18.12.2021.
//
#ifndef ServerConfigureEventTestSuite_h
#define ServerConfigureEventTestSuite_h

#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME ServerConfigureEventTestSuite

template <typename TEvent>
struct EventCallback : public Tango::CallBack
{
    EventCallback()
        : num_of_all_events(0)
        , num_of_error_events(0)
    {}

    void push_event(TEvent* event)
    {
        num_of_all_events++;
        if (event->err)
        {
            num_of_error_events++;
        }
    }

    int num_of_all_events;
    int num_of_error_events;
};

class ServerConfigureEventTestSuite : public CxxTest::TestSuite {
protected:
    string dserver_name, device1_name, device1_instance_name;
    string event_port, event_port_str, heartbeat_port, heartbeat_port_str;
    string::size_type pos;

public:
    SUITE_NAME():
    device1_instance_name{"test"}
    {

//
// Arguments check -------------------------------------------------
//
        event_port = "9977";
        heartbeat_port = "9988";
        device1_name = CxxTest::TangoPrinter::get_param("device1");
        dserver_name = "dserver/" + CxxTest::TangoPrinter::get_param("fulldsname");
        CxxTest::TangoPrinter::validate_args();
    }

    virtual ~SUITE_NAME()
    {
        // Cleanup servers if necessary
        CxxTest::TangoPrinter::kill_server();

        // make sure that device1 is actually running as it expected
        // by subsequent tests
        CxxTest::TangoPrinter::start_server(device1_instance_name);
    }

    static SUITE_NAME *createSuite()
    {
        return new SUITE_NAME();
    }

    static void destroySuite(SUITE_NAME *suite)
    {
        delete suite;
    }

    // setup: ensure that server is torndown before each test
    void setUp()
    {
        TEST_LOG << endl << "cxx_server_configure_zmq_ports: setup" << endl;
        // Stop any previously running server first
        CxxTest::TangoPrinter::kill_server();
    }

    // startup device server with optional fixed ports
    void start_server_with_ports(std::string event_port, std::string heartbeat_port)
    {

        std::string env = "/usr/bin/env ";
        if (! event_port.empty())
        {
            env = env+" TANGO_ZMQ_EVENT_PORT="+event_port+" ";
        }
        if (! event_port.empty())
        {
            env = env+" TANGO_ZMQ_HEARTBEAT_PORT="+heartbeat_port+" ";
        }
        // Restart the test server with env vars for the fixed ports
        TEST_LOG << endl << "cxx_server_configure_zmq_ports: relaunching DevTest/test with port specification - " << env + Tango::kStartServerCmd + device1_instance_name << endl;
        TS_ASSERT_THROWS_NOTHING(
            system((env + Tango::kStartServerCmd + device1_instance_name).c_str());
        );
        CxxTest::TangoPrinter::restore_set("test/debian8/10 started.");

        // wait a bit for the server to properly start
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }


    // create and check device1 proxy
    DeviceProxy* create_and_check_device1_proxy()
    {
        // connect and ping device1
        DeviceProxy *device1;
        bool res_ping_device1 = true;
        device1 = new DeviceProxy(device1_name);
        TS_ASSERT_THROWS_NOTHING(
            res_ping_device1 = device1->ping();
        );
        TEST_LOG << endl << "cxx_server_configure_zmq_ports: pinged device1 - "+device1_name << endl;
        TS_ASSERT(res_ping_device1);
        return device1;
    }


    // create and check admin device proxy
    DeviceProxy* create_and_check_admin_device_proxy()
    {
        // connect and ping admin device
        DeviceProxy *dserver;
        bool res_ping_dserver = true;
        dserver = new DeviceProxy(dserver_name);
        TS_ASSERT_THROWS_NOTHING(
            res_ping_dserver = dserver->ping();
        );
        TEST_LOG << endl << "cxx_server_configure_zmq_ports: pinged admin device - "+dserver_name << endl;
        TS_ASSERT(res_ping_dserver);
        return dserver;
    }


    // exercise the subscription event
    void exercise_subscription_event(DeviceProxy* device1)
    {
        // setup a subscription so that the event port is woken up
        EventCallback<Tango::EventData> callback{};
        std::string attribute_name = "event_change_tst";
        TS_ASSERT_THROWS_NOTHING(device1->subscribe_event(
            attribute_name,
            Tango::USER_EVENT,
            &callback));
        TEST_LOG << endl << "cxx_server_configure_zmq_ports: sending IOPushEvent" << endl;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOPushEvent"));
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // query admin server and return admin server info
    std::vector<std::string> query_admin_device(DeviceProxy* dserver)
    {
        // query the admin device so that we get the zmq port specifications
        TEST_LOG << endl << "cxx_server_configure_zmq_ports: sending ZmqEventSubscriptionChange" << endl << endl;
        DeviceData din,dout;
        vector<string> din_info;
        din_info.push_back("info");
        din << din_info;
        TS_ASSERT_THROWS_NOTHING(dout = dserver->command_inout("ZmqEventSubscriptionChange", din));

        // extract and check the zmq ports for events and heartbeat
        const DevVarLongStringArray *result;
        dout >> result;
        std::vector<std::string> endpoints {};

        for (CORBA::ULong i = 0;i < result->svalue.length();i++)
        {
            TEST_LOG << endl << "cxx_server_configure_zmq_ports: info["+std::to_string(i)+"] => "+string(result->svalue[i]) << endl;
            endpoints.push_back(string(result->svalue[i]));
        }
        return endpoints;
    }

//
// Tests -------------------------------------------------------
//


//
// Ask the device server to subscribe to an event and then check the
// ZmqEventSubscriptionChange ["info"] results to see whether it has
// ephemeral event and heartbeat ports on the DS
//
    void test_01_device_server_zmq_event_port_ephemeral(void)
    {
        // Stop any previously running server first

        start_server_with_ports(string(""), string(""));

        DeviceProxy *device1 = create_and_check_device1_proxy();
        DeviceProxy *dserver = create_and_check_admin_device_proxy();

        TEST_LOG << endl << "test_device_server_zmq_event_port_ephemeral: new DeviceProxy(" << device1->name() +"[" + device1_name +"]" << ") returned" << endl;

        // setup a subscription so that the event port is woken up
        exercise_subscription_event(device1);
        std::vector<std::string> result = query_admin_device(dserver);
        TS_ASSERT(result.size() > 1);
        std::string heartbeat_endpoint = result[0];
        std::string event_endpoint = result[1];
        TEST_LOG << endl << "test_device_server_zmq_event_port_ephemeral: finished" << endl << endl;
        TEST_LOG << "Heartbeat endpoint: " << heartbeat_endpoint << endl;
        TEST_LOG << "Event endpoint: " << event_endpoint << endl;

        // Check that the ports are not equal to the fixed ones ie. ephemeral
        // and that they are in the ephemeral range - default 32768 to 60999
        pos = heartbeat_endpoint.rfind(':');
        heartbeat_port_str = heartbeat_endpoint.substr(pos + 1);
        TS_ASSERT(heartbeat_port != heartbeat_port_str);
        int heartbeat_port_int = std::stoi(heartbeat_port_str);
        TS_ASSERT(heartbeat_port_int >= 32768 && heartbeat_port_int <= 60999)
        pos = event_endpoint.rfind(':');
        event_port_str = event_endpoint.substr(pos + 1);
        TS_ASSERT(event_port != event_port_str);
        int event_port_int = std::stoi(event_port_str);
        TS_ASSERT(event_port_int >= 32768 && event_port_int <= 60999)

        // Cleanup device proxies
        delete device1;
        delete dserver;
    }

//
// Ask the device server to subscribe to an event and then check the
// ZmqEventSubscriptionChange ["info"] results to see whether it has
// the fixed event and heartbeat ports on the DS
//
    void test_02_device_server_zmq_event_port_configured(void)
    {

        start_server_with_ports(event_port, heartbeat_port);

        DeviceProxy *device1 = create_and_check_device1_proxy();
        DeviceProxy *dserver = create_and_check_admin_device_proxy();

        TEST_LOG << endl << "test_device_server_zmq_event_port_configured: new DeviceProxy(" << device1->name() +"[" + device1_name +"]" << ") returned" << endl;

        // setup a subscription so that the event port is woken up
        exercise_subscription_event(device1);
        std::vector<std::string> result = query_admin_device(dserver);
        TS_ASSERT(result.size() > 1);
        std::string heartbeat_endpoint = result[0];
        std::string event_endpoint = result[1];
        TEST_LOG << endl << "test_device_server_zmq_event_port_configured: finished" << endl << endl;
        TEST_LOG << "Heartbeat endpoint: " << heartbeat_endpoint << endl;
        TEST_LOG << "Event endpoint: " << event_endpoint << endl;

        pos = heartbeat_endpoint.rfind(':');
        heartbeat_port_str = heartbeat_endpoint.substr(pos + 1);
        TS_ASSERT_EQUALS(heartbeat_port, heartbeat_port_str);
        pos = event_endpoint.rfind(':');
        event_port_str = event_endpoint.substr(pos + 1);
        TS_ASSERT_EQUALS(event_port, event_port_str);

        // Cleanup device proxies
        delete device1;
        delete dserver;
    }

//
// Specify an invalid event port specification XXXX - expect an error when
// subscribing to an event on this device server.
// Query the admin device to check that the expected port values were
// set for event and heartbeat.
//
    void test_03_device_server_zmq_event_invalid_event_port_configured(void)
    {

        std::string bogus_event_port("XXXX");
        start_server_with_ports(bogus_event_port, heartbeat_port);

        DeviceProxy *device1 = create_and_check_device1_proxy();
        DeviceProxy *dserver = create_and_check_admin_device_proxy();

        // setup a subscription so that the event port is woken up - check it that it fails
        TEST_LOG << endl << "test_device_server_zmq_event_invalid_port_configured: new DeviceProxy(" << device1->name() +"[" + device1_name +"]" << ") returned" << endl;

        EventCallback<Tango::EventData> callback{};
        std::string attribute_name = "event_change_tst";
        Tango::DevFailed erescue;
        TS_ASSERT_THROWS_ASSERT(device1->subscribe_event(
            attribute_name,
            Tango::USER_EVENT,
            &callback),
            Tango::DevFailed &e,
            erescue = e;
        );
        // Except::print_exception(erescue);
        // Check the resulting exception was correct
        TS_ASSERT(erescue.errors.length() >= 1);
        TS_ASSERT_EQUALS(std::string(erescue.errors[0].reason.in()), "API_ZmqInitFailed");
        // query the admin device and check the zmq ports were set as expected
        std::vector<std::string> result = query_admin_device(dserver);
        TS_ASSERT(result.size() > 1);
        std::string heartbeat_endpoint = result[0];
        std::string event_endpoint = result[1];
        TEST_LOG << "Heartbeat endpoint: " << heartbeat_endpoint << endl;
        TEST_LOG << "Event endpoint: " << event_endpoint << endl;
        pos = heartbeat_endpoint.rfind(':');
        heartbeat_port_str = heartbeat_endpoint.substr(pos + 1);
        TS_ASSERT_EQUALS(heartbeat_port, heartbeat_port_str);
        TS_ASSERT_EQUALS(event_endpoint, "Event: tcp://*:"+bogus_event_port);

        // Cleanup device proxies
        delete device1;
        delete dserver;
    }


//
// Specify an invalid heartbeat port specification YYYY - basically
// nothing works because the server will fail to start
//
    void test_04_device_server_zmq_event_invalid_heartbeat_port_configured(void)
    {

        DeviceProxy *device1;

        start_server_with_ports(event_port, string("YYYY"));

        // try and ping the device - this should fail because of bad heartbeat config
        device1 = new DeviceProxy(device1_name);
        Tango::DevFailed erescue;
        TS_ASSERT_THROWS_ASSERT(
            device1->ping(),
            Tango::DevFailed &e,
            erescue = e;
        );
        TS_ASSERT(erescue.errors.length() >= 1);
        TS_ASSERT_EQUALS(std::string(erescue.errors[0].reason.in()), "API_DeviceNotExported");

        // connect and ping admin device - this should fail because of bad heartbeat config
        DeviceProxy *dserver;
        dserver = new DeviceProxy(dserver_name);
        TS_ASSERT_THROWS_ASSERT(
            dserver->ping(),
            Tango::DevFailed &e,
            erescue = e;
        );
        TS_ASSERT(erescue.errors.length() >= 1);
        TS_ASSERT_EQUALS(std::string(erescue.errors[0].reason.in()), "API_DeviceNotExported");

        // send a subscription event - this should fail because of bad heartbeat config
        DeviceData din,dout;
        vector<string> din_info;
        din_info.push_back("info");
        din << din_info;
        TS_ASSERT_THROWS_ASSERT(dout = dserver->command_inout("ZmqEventSubscriptionChange", din),
            Tango::DevFailed &e,
            erescue = e;
        );
        TS_ASSERT(erescue.errors.length() >= 1);
        TS_ASSERT_EQUALS(std::string(erescue.errors[0].reason.in()), "API_CantConnectToDevice");

        // Cleanup device proxies
        delete device1;
        delete dserver;
    }
};

#endif // ServerConfigureEventTestSuite_h
