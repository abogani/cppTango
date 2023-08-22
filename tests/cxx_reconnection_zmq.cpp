//
// Created by ingvord on 12/14/16.
//
#ifndef RecoZmqTestSuite_h
#define RecoZmqTestSuite_h

#include <thread>
#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME RecoZmqTestSuite

class RecoZmqTestSuite : public CxxTest::TestSuite
{
  protected:
    DeviceProxy *device1, *device2;
    string device1_name, device2_name, device1_instance_name, device2_instance_name;
    CountingCallBack<Tango::EventData> eventCallback;

  public:
    SUITE_NAME() :
        device1_instance_name{"test"}, // TODO pass via cl
        device2_instance_name{"test2"}
    {
        //
        // Arguments check -------------------------------------------------
        //

        device1_name = CxxTest::TangoPrinter::get_param("device1");
        device2_name = CxxTest::TangoPrinter::get_param("device20");

        CxxTest::TangoPrinter::validate_args();

        //
        // Initialization --------------------------------------------------
        //

        try
        {
            device1 = new DeviceProxy(device1_name);
            device2 = new DeviceProxy(device2_name);

            // TODO start server 2 and set fallback point
            CxxTest::TangoPrinter::start_server(device2_instance_name);
            CxxTest::TangoPrinter::restore_set("test2/debian8/20 started.");

            // sleep 18 &&  start_server "@INST_NAME@" &
            thread(
                [this]()
                {
                    std::this_thread::sleep_for(std::chrono::seconds(18));
                    CxxTest::TangoPrinter::start_server(device1_instance_name);
                })
                .detach();

            // sleep 62 &&  start_server "@INST_NAME@" &
            thread(
                [this]()
                {
                    std::this_thread::sleep_for(std::chrono::seconds(62));
                    CxxTest::TangoPrinter::start_server(device1_instance_name);
                })
                .detach();
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
    }

    virtual ~SUITE_NAME()
    {
        if(CxxTest::TangoPrinter::is_restore_set("test2/debian8/20 started."))
        {
            CxxTest::TangoPrinter::kill_server();
        }

        CxxTest::TangoPrinter::start_server(device1_instance_name);

        delete device1;
        delete device2;
    }

    static SUITE_NAME *createSuite()
    {
        return new SUITE_NAME();
    }

    static void destroySuite(SUITE_NAME *suite)
    {
        delete suite;
    }

    //
    // Tests -------------------------------------------------------
    //

    //
    // Subscribe to a user event
    //
    void test_subscribe_to_user_event(void)
    {
        string att_name("event_change_tst");

        const vector<string> filters;
        eventCallback.reset_counts();

        TS_ASSERT_THROWS_NOTHING(device1->subscribe_event(att_name, Tango::USER_EVENT, &eventCallback, filters));

        //
        // Fire one event
        //

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOPushEvent"));
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOPushEvent"));

        eventCallback.wait_for([&]() { return eventCallback.invocation_count() >= 3; });

        TEST_LOG << "Callback execution before re-connection = " << eventCallback.invocation_count() << endl;
        TEST_LOG << "Callback error before re-connection = " << eventCallback.error_count() << endl;

        TS_ASSERT_EQUALS(3, eventCallback.invocation_count());
        TS_ASSERT_EQUALS(0, eventCallback.error_count());

        //
        // Kill device server (using its admin device)
        //

        string adm_name = device1->adm_name();
        DeviceProxy admin_dev(adm_name);
        TS_ASSERT_THROWS_NOTHING(admin_dev.command_inout("kill"));

        //
        // Wait for some error and re-connection
        //
        eventCallback.wait_for([&]() { return eventCallback.success_count() >= 4; });

        //
        // Check error and re-connection
        //

        TEST_LOG << "Callback execution after re-connection = " << eventCallback.invocation_count() << endl;
        TEST_LOG << "Callback error after re-connection = " << eventCallback.error_count() << endl;

        TS_ASSERT_LESS_THAN_EQUALS(1, eventCallback.error_count());
        TS_ASSERT_EQUALS(4, eventCallback.success_count());

        //
        // Fire another event
        //

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOPushEvent"));
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOPushEvent"));

        eventCallback.wait_for([&]() { return eventCallback.success_count() >= 6; });

        TEST_LOG << "Callback execution after re-connection and event = " << eventCallback.invocation_count() << endl;
        TEST_LOG << "Callback error after re-connection and event = " << eventCallback.error_count() << endl;

        TS_ASSERT_EQUALS(6, eventCallback.success_count());
    }

    //
    // Clear call back counters and kill device server once more
    //
    void test_clear_cb_kill_ds(void)
    {
        eventCallback.reset_counts();

        string adm_name = device1->adm_name();
        DeviceProxy admin_dev(adm_name);
        TS_ASSERT_THROWS_NOTHING(admin_dev.command_inout("kill"));

        //
        // Wait for some error and re-connection
        //

        eventCallback.wait_for([&]() { return eventCallback.success_count() >= 1; });

        //
        // Check error and re-connection
        //

        TEST_LOG << "Callback execution after second re-connection = " << eventCallback.invocation_count() << endl;
        TEST_LOG << "Callback error after second re-connection = " << eventCallback.error_count() << endl;

        TS_ASSERT_LESS_THAN_EQUALS(1, eventCallback.error_count());
        TS_ASSERT_EQUALS(1, eventCallback.success_count());

        //
        // Fire yet another event
        //

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOPushEvent"));

        eventCallback.wait_for([&]() { return eventCallback.success_count() >= 2; });

        TEST_LOG << "Callback execution after second re-connection and event = " << eventCallback.invocation_count()
                 << endl;
        TEST_LOG << "Callback error after second re-connection and event = " << eventCallback.error_count() << endl;

        TS_ASSERT_EQUALS(2, eventCallback.success_count());
        TS_ASSERT_LESS_THAN_EQUALS(1, eventCallback.error_count());
    }
};

#endif // RecoZmqTestSuite_h
