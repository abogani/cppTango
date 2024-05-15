// NOLINTBEGIN(*)

//
// Created by ingvord on 12/14/16.
//

#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME ServerEventTestSuite

class ServerEventTestSuite : public CxxTest::TestSuite
{
  protected:
    DeviceProxy *device1, *device2;
    string device1_name, device2_name, device1_instance_name, device2_instance_name, full_ds_name;
    DevLong eve_id;

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
        full_ds_name = CxxTest::TangoPrinter::get_param("fulldsname");

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
        }
        catch(std::runtime_error &ex)
        {
            std::cerr << "start_server failed: \"" << ex.what() << "\"\n";
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
            try
            {
                CxxTest::TangoPrinter::kill_server();
            }
            catch(const std::runtime_error &ex)
            {
                std::cerr << "kill_server failed during teardown: \"" << ex.what() << "\"\n";
            }
        }

        try
        {
            CxxTest::TangoPrinter::start_server(device1_instance_name);
        }
        catch(const std::runtime_error &ex)
        {
            std::cerr << "start_server failed during teardown: \"" << ex.what() << "\"\n";
        }

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
    // Ask the device server to subscribe to an event
    //
    void test_device_server_subscribe_to_event(void)
    {
        TEST_LOG << endl << "new DeviceProxy(" << device1->name() << ") returned" << endl << endl;

        vector<string> vs{device2_name, "Short_attr", "periodic"};

        DeviceData dd_in, dd_out;
        dd_in << vs;
        TS_ASSERT_THROWS_NOTHING(dd_out = device1->command_inout("IOSubscribeEvent", dd_in));
        dd_out >> eve_id;
    }

    //
    // Wait for event to be executed
    //

    void test_wait_event(void)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        DeviceData da;
        TS_ASSERT_THROWS_NOTHING(da = device1->command_inout("IOGetCbExecuted"));
        Tango::DevLong cb;
        da >> cb;

        TEST_LOG << "cb executed = " << cb << endl;
        TS_ASSERT_LESS_THAN_EQUALS(2, cb);
        TS_ASSERT_LESS_THAN_EQUALS(cb, 4);
    }

    //
    // Ask server to unsubsribe from event
    //
    void test_server_unsubscribes_from_event(void)
    {
        DeviceData dd_un;
        dd_un << eve_id;

        DeviceData da;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOUnSubscribeEvent", dd_un));
        TS_ASSERT_THROWS_NOTHING(da = device1->command_inout("IOGetCbExecuted"));

        Tango::DevLong cb;
        da >> cb;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        TS_ASSERT_THROWS_NOTHING(da = device1->command_inout("IOGetCbExecuted"));
        Tango::DevLong cb2;
        da >> cb2;

        TS_ASSERT_EQUALS(cb2, cb);
    }

    /**
     * Tests that the client can still receive events after the device server is
     * shut down, renamed in the database and then restarted. This scenario used
     * to fail as reported in #679.
     */
    void test_reconnection_after_ds_instance_rename()
    {
        const std::string new_instance_name = "renamed_ds";
        const std::string old_ds_name = full_ds_name;

        size_t exec_part_end = full_ds_name.find("/");
        std::string new_ds_name = full_ds_name.substr(0, exec_part_end + 1);
        new_ds_name += new_instance_name;

        const std::string attribute_name = "event_change_tst";

        CountingCallBack<Tango::EventData> callback{};

        int subscription{};
        TS_ASSERT_THROWS_NOTHING(subscription = device1->subscribe_event(attribute_name, Tango::USER_EVENT, &callback));

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOPushEvent"));
        callback.wait_for([&]() { return callback.invocation_count() >= 2; });
        TS_ASSERT_EQUALS(2, callback.invocation_count());
        TS_ASSERT_EQUALS(0, callback.error_count());

        TS_ASSERT_THROWS_NOTHING(CxxTest::TangoPrinter::kill_server());

        Database db{};
        db.rename_server(old_ds_name, new_ds_name);

        TS_ASSERT_THROWS_NOTHING(CxxTest::TangoPrinter::start_server(new_instance_name));
        // std::this_thread::sleep_for(std::chrono::seconds(EVENT_HEARTBEAT_PERIOD)); // Wait for reconnection

        callback.wait_for([&]() { return callback.invocation_count() >= 5; });
        TS_ASSERT_EQUALS(4, callback.invocation_count());
        TS_ASSERT_EQUALS(1, callback.error_count());

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOPushEvent"));

        callback.wait_for([&]() { return callback.invocation_count() >= 6; });
        TS_ASSERT_EQUALS(5, callback.invocation_count());
        TS_ASSERT_EQUALS(1, callback.error_count());

        TS_ASSERT_THROWS_NOTHING(device1->unsubscribe_event(subscription));

        db.rename_server(new_ds_name, old_ds_name);
    }
};

// NOLINTEND(*)
