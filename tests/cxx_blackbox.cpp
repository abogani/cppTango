// NOLINTBEGIN(*)

#ifdef _TG_WINDOWS_
  #include <process.h> // needed to obtain process id
#endif                 // _TG_WINDOWS_

#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME BlackboxTestSuite

class BlackboxTestSuite : public CxxTest::TestSuite
{
  protected:
    DeviceProxy *device1, *device2, *device3, *dserver, *dbserver;
    string device1_name, device2_name, device3_name, fulldsname, client_host, dbserver_name, dserver_name;
    DevLong server_version;

  public:
    SUITE_NAME()
    {
        //
        // Arguments check -------------------------------------------------
        //

        device1_name = CxxTest::TangoPrinter::get_param("device1");
        device2_name = CxxTest::TangoPrinter::get_param("device2");
        device3_name = CxxTest::TangoPrinter::get_param("device3");
        dserver_name = "dserver/" + CxxTest::TangoPrinter::get_param("fulldsname");
        client_host = CxxTest::TangoPrinter::get_param("clienthost");
        TS_ASSERT_THROWS_NOTHING(server_version = parse_as<int>(CxxTest::TangoPrinter::get_param("serverversion")));
        dbserver_name = CxxTest::TangoPrinter::get_param("dbserver");

        CxxTest::TangoPrinter::validate_args();

        //
        // Initialization --------------------------------------------------
        //

        try
        {
            device1 = new DeviceProxy(device1_name);
            device2 = new DeviceProxy(device2_name);
            dserver = new DeviceProxy(dserver_name);
            dbserver = new DeviceProxy(dbserver_name);
            device1->ping();
            device2->ping();
            dserver->ping();
            dbserver->ping();
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
    }

    virtual ~SUITE_NAME()
    {
        // set the Tango::ON state on suite tearDown() in case a test fails leaving Tango::OFF status
        DeviceData din;
        din << device1_name;
        try
        {
            dserver->set_timeout_millis(5000);
            dserver->command_inout("DevRestart", din);
        }
        catch(CORBA::Exception &e)
        {
            TEST_LOG << endl << "Exception in suite tearDown():" << endl;
            Except::print_exception(e);
            exit(-1);
        }

        delete device1;
        delete device2;
        delete dserver;
        delete dbserver;

        std::this_thread::sleep_for(std::chrono::seconds(5));
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

    // Test black-box device feature

    void test_blackbox_device_feature(void)
    {
        TS_ASSERT_THROWS_ASSERT(device1->black_box(0),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_BlackBoxArgument);
                                TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));

        DeviceData din, dout;
        DevLong lg_in = 10, lg_out;

        din << lg_in;
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("IOLong", din));
        dout >> lg_out;
        TS_ASSERT_EQUALS(lg_out, 20);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("IOLong", din));
        dout >> lg_out;
        TS_ASSERT_EQUALS(lg_out, 20);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("IOLong", din));
        dout >> lg_out;
        TS_ASSERT_EQUALS(lg_out, 20);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("IOLong", din));
        dout >> lg_out;
        TS_ASSERT_EQUALS(lg_out, 20);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("IOLong", din));
        dout >> lg_out;
        TS_ASSERT_EQUALS(lg_out, 20);

        vector<string> *blackbox_out = nullptr;
        string out_str, version_str, reference_str, pid_str;

        // sets the "command_inout" suffix (refer to blackbox.cpp build_info_as_str())
        switch(server_version)
        {
        case 2:
            version_str = "_2";
            break;
        case 3:
            version_str = "_2";
            break;
        case 4:
        case 5:
        case 6:
            version_str = "_4";
            break;
        default:
            TS_FAIL("Unexpected default");
        }

#ifdef _TG_WINDOWS_
        int pid = _getpid();
#else
        pid_t pid = getpid();
#endif // _TG_WINDOWS_

        stringstream ss;
        ss << pid;
        pid_str = ss.str();

        reference_str = "Operation command_inout" + version_str + " (cmd = IOLong) from cache_device requested from " +
                        client_host + " (CPP/Python client with PID " + pid_str + ")";

        TS_ASSERT_THROWS_NOTHING(blackbox_out = device1->black_box(3));
        for(vector<string>::iterator it = (*blackbox_out).begin(); it != (*blackbox_out).end(); ++it)
        {
            out_str = *it;
            out_str.erase(0, out_str.rfind(": ") + 2); // removes time stamp from the output

            TS_ASSERT_EQUALS(out_str, reference_str);
        }
        delete blackbox_out;

        // removing properties of memorized attributes
        string query =
            "select attribute, name  from property_attribute_device where name like '\\_\\_%' and  device='" +
            device3_name + "'";
        const DevVarLongStringArray *res;
        DevVarLongStringArray result;
        din << query;
        TS_ASSERT_THROWS_NOTHING(dout = dbserver->command_inout("DbMySqlSelect", din));
        dout >> res;
        result = *res;
        string attr_name, attr_prop;
        // TODO: check the loop
        for(unsigned int i = 0; i < result.svalue.length(); i++)
        {
            if(i % 2 == 0)
            {
                //                TEST_LOG << "===> attr_name: " << string(result.svalue[i].in()) << endl;
                attr_name = device3_name + "/" + string(result.svalue[i].in());
            }
            else
            {
                //                TEST_LOG << "===> attr_prop: " << string(result.svalue[i].in()) << endl;
                attr_prop = string(result.svalue[i].in());

                AttributeProxy *my_attr = nullptr;
                TS_ASSERT_THROWS_NOTHING(my_attr = new AttributeProxy(attr_name));
                TS_ASSERT_THROWS_NOTHING(my_attr->delete_property(attr_prop));
                delete my_attr;
            }
        }

        TS_ASSERT_THROWS_NOTHING(dserver->command_inout("RestartServer"));
        std::this_thread::sleep_for(std::chrono::seconds(3));

        TS_ASSERT_THROWS_NOTHING(device3 = new DeviceProxy(device3_name));

        TEST_LOG << "Start of strange test" << endl;
        try
        {
            vector<string> *bb = device3->black_box(5);
            TEST_LOG << endl << "===> blackbox size: " << (*bb).size() << endl;
            if((*bb).size() > 0)
            {
                TEST_LOG << "===> first element: " << (*bb)[0] << endl;
            }
            delete bb;
        }
        catch(DevFailed &e)
        {
            string reas(e.errors[0].reason.in());
            TEST_LOG << "Exception errors length = " << e.errors.length() << endl;
            TEST_LOG << "Exception reason = " << reas << endl;
            TEST_LOG << "Exception desc = " << e.errors[0].desc.in() << endl;
            TEST_LOG << "Exception origin = " << e.errors[0].origin.in() << endl;
            if(e.errors.length() > 1)
            {
                TEST_LOG << "Exception 1 reason = " << e.errors[1].reason.in() << endl;
                TEST_LOG << "Exception 1 desc = " << e.errors[1].desc.in() << endl;
                TEST_LOG << "Exception 1 origin = " << e.errors[1].origin.in() << endl;
            }
            Tango::DeviceProxy *dev = new Tango::DeviceProxy(dserver_name);
            try
            {
                Tango::DeviceData dd = dev->command_inout("QueryDevice");
                TEST_LOG << "Device list = " << dd << endl;
            }
            catch(Tango::DevFailed &)
            {
                TEST_LOG << "Again exception when talking to adm device!!!" << endl;
            }
            TEST_LOG << "===> Nothing yet stored in blackbox, error reason = " << reas << endl;
            //            TS_ASSERT_EQUALS(reas, API_BlackBoxEmpty);
            if(reas == API_CorbaException)
            {
                TEST_LOG << "Too early, sleeping 4 more seconds...." << endl;
                std::this_thread::sleep_for(std::chrono::seconds(4));

                try
                {
                    vector<string> *bb = device3->black_box(5);
                    TEST_LOG << endl << "===> blackbox size: " << (*bb).size() << endl;
                    if((*bb).size() > 0)
                    {
                        TEST_LOG << "===> first element: " << (*bb)[0] << endl;
                    }
                }
                catch(DevFailed &e)
                {
                    string reas(e.errors[0].reason.in());
                    TEST_LOG << "Exception reason = " << reas << endl;
                    TEST_LOG << "Exception desc = " << e.errors[0].desc.in() << endl;
                    TEST_LOG << "Exception origin = " << e.errors[0].origin.in() << endl;
                    if(e.errors.length() > 1)
                    {
                        TEST_LOG << "Exception 1 reason = " << e.errors[1].reason.in() << endl;
                        TEST_LOG << "Exception 1 desc = " << e.errors[1].desc.in() << endl;
                        TEST_LOG << "Exception 1 origin = " << e.errors[1].origin.in() << endl;
                    }
                    Tango::DeviceProxy *dev = new Tango::DeviceProxy(dserver_name);
                    try
                    {
                        Tango::DeviceData dd = dev->command_inout("QueryDevice");
                        TEST_LOG << "Device list = " << dd << endl;
                    }
                    catch(Tango::DevFailed &)
                    {
                        TEST_LOG << "Again exception when talking to adm device!!!" << endl;
                    }
                    TEST_LOG << "===> Nothing yet stored in blackbox, error reason = " << reas << endl;
                    TS_ASSERT_EQUALS(reas, API_BlackBoxEmpty);
                    delete dev;
                }
            }
            else
            {
                TS_ASSERT_EQUALS(reas, API_BlackBoxEmpty);
            }

            delete dev;
        }
        catch(...)
        {
            TEST_LOG << "===> Error in checking blackbox size" << endl;
            TS_ASSERT(false);
        }

        /*        TS_ASSERT_THROWS_ASSERT(device3->black_box(2), Tango::DevFailed &e,
                        TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_BlackBoxEmpty);
                        TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));*/
        delete device3;

        reference_str = "Operation info requested from " + client_host;
        TS_ASSERT_THROWS_NOTHING(blackbox_out = device2->black_box(1));
        out_str = (*blackbox_out)[0];
        out_str.erase(0, out_str.rfind(": ") + 2);
        TS_ASSERT_EQUALS(out_str, reference_str);
        delete blackbox_out;
    }
};

// NOLINTEND(*)
