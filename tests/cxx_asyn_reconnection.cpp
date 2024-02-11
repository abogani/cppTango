// NOLINTBEGIN(*)

#ifndef AsynReconnectionTestSuite_h
  #define AsynReconnectionTestSuite_h

  #include "cxx_common.h"

  #undef SUITE_NAME
  #define SUITE_NAME AsynReconnectionTestSuite

class AsynReconnectionTestSuite : public CxxTest::TestSuite
{
  protected:
    DeviceProxy *device1;
    string device1_name;
    string device1_instance_name;

  public:
    SUITE_NAME() :
        device1_instance_name{"test"} // TODO pass via cl
    {
        //
        // Arguments check -------------------------------------------------
        //

        // locally defined (test suite scope) mandatory parameters
        // localparam = CxxTest::TangoPrinter::get_param_loc("localparam","description of what localparam is");

        // predefined mandatory parameters
        device1_name = CxxTest::TangoPrinter::get_param("device1");

        // predefined optional parameters
        // CxxTest::TangoPrinter::get_param_opt("loop"); // loop parameter is then managed by the CXX framework itself

        // always add this line, otherwise arguments will not be parsed correctly
        CxxTest::TangoPrinter::validate_args();

        //
        // Initialization --------------------------------------------------
        //

        try
        {
            device1 = new DeviceProxy(device1_name);
            device1->ping();
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
    }

    virtual ~SUITE_NAME()
    {
        //
        // Clean up --------------------------------------------------------
        //

        // clean up in case test suite terminates before Server_Killed is restored to defaults
        if(CxxTest::TangoPrinter::is_restore_set("Server_Killed"))
        {
            try
            {
                CxxTest::TangoPrinter::start_server(device1_instance_name);
            }
            catch(const std::runtime_error &ex)
            {
                std::cerr << "start_server failed: \"" << ex.what() << "\"\n";
            }
        }

        // delete dynamically allocated objects
        delete device1;
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

    // Test TestNormalWriteAttributeAsync

    void test_TestNormalWriteAttributeAsynch()
    {
        try
        {
            // Write one attribute
            long id = 0;
            DeviceAttribute send;

            send.set_name("attr_asyn_write");
            DevLong lg = 222;
            TS_ASSERT_THROWS_NOTHING(send << lg);

            TS_ASSERT_THROWS_NOTHING(id = device1->write_attribute_asynch(send));

            bool finish = false;
            long nb_not_arrived = 0;
            while(finish == false)
            {
                try
                {
                    device1->write_attribute_reply(id);
                    finish = true;
                }
                catch(AsynReplyNotArrived &)
                {
                    finish = false;
                    TEST_LOG << "Attribute not yet written" << endl;
                    nb_not_arrived++;
                }
                if(finish == false)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }

            TS_ASSERT(nb_not_arrived >= 1);

            TEST_LOG << "   Asynchronous write_attribute in polling mode --> OK" << endl;
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            exit(-1);
        }
        catch(CORBA::Exception &ex)
        {
            Except::print_exception(ex);
            exit(-1);
        }
    }

    // Test TestWriteAttributeAsynchAfterReconnection

    void test_TestWriteAttributeAsynchAfterReconnection()
    {
        TS_ASSERT_THROWS_NOTHING(CxxTest::TangoPrinter::kill_server());
        CxxTest::TangoPrinter::restore_set("Server_Killed");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        TS_ASSERT_THROWS_NOTHING(CxxTest::TangoPrinter::start_server(device1_instance_name));
        std::this_thread::sleep_for(std::chrono::seconds(1));
        CxxTest::TangoPrinter::restore_unset("Server_Killed");
        try
        {
            // Write one attribute
            long id = 0;
            DeviceAttribute send;

            send.set_name("attr_asyn_write");
            DevLong lg = 444;
            TS_ASSERT_THROWS_NOTHING(send << lg);

            TS_ASSERT_THROWS_NOTHING(id = device1->write_attribute_asynch(send));

            bool finish = false;
            long nb_not_arrived = 0;
            while(finish == false)
            {
                try
                {
                    device1->write_attribute_reply(id);
                    finish = true;
                }
                catch(AsynReplyNotArrived &)
                {
                    finish = false;
                    TEST_LOG << "Attribute not yet written" << endl;
                    nb_not_arrived++;
                }
                if(finish == false)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }

            TS_ASSERT(nb_not_arrived >= 1);

            // Read the attribute
            long read_id;
            DeviceAttribute *received = nullptr;

            read_id = device1->read_attribute_asynch("attr_asyn_write");

            finish = false;
            nb_not_arrived = 0;
            while(finish == false)
            {
                try
                {
                    received = device1->read_attribute_reply(read_id);
                    Tango::DevLong val;
                    *received >> val;
                    TEST_LOG << "attr_asyn_write attribute value = " << val << endl;
                    TS_ASSERT_EQUALS(val, 444);
                    finish = true;
                }
                catch(AsynReplyNotArrived &)
                {
                    finish = false;
                    TEST_LOG << "Attribute not yet read" << endl;
                    nb_not_arrived++;
                }
                if(finish == false)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            delete received;

            TS_ASSERT(nb_not_arrived >= 1);

            TEST_LOG << "   Asynchronous read_attribute in polling mode --> OK" << endl;

            TEST_LOG << "   Asynchronous write_attribute in polling mode after reconnection--> OK" << endl;
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            exit(-1);
        }
        catch(CORBA::Exception &ex)
        {
            Except::print_exception(ex);
            exit(-1);
        }
    }
};
#endif // AsynReconnectionTestSuite_h

// NOLINTEND(*)
