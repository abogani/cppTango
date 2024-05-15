// NOLINTBEGIN(*)

#include <tango/server/tango_current_function.h>

#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME AttrMiscTestSuite

class AttrMiscTestSuite : public CxxTest::TestSuite
{
  protected:
    DeviceProxy *device1, *dserver;
    string device1_name;
    AttributeInfoListEx *init_attr_conf;
    int def_timeout;

  public:
    SUITE_NAME()
    {
        // default timeout
        def_timeout = 3000;

        //
        // Arguments check -------------------------------------------------
        //

        string dserver_name;

        device1_name = CxxTest::TangoPrinter::get_param("device1");
        dserver_name = "dserver/" + CxxTest::TangoPrinter::get_param("fulldsname");

        CxxTest::TangoPrinter::validate_args();

        //
        // Initialization --------------------------------------------------
        //

        try
        {
            device1 = new DeviceProxy(device1_name);
            dserver = new DeviceProxy(dserver_name);
            device1->ping();
            dserver->ping();

            dserver->command_inout("RestartServer");
            std::this_thread::sleep_for(std::chrono::seconds(10));

            vector<string> attr_list;
            attr_list.push_back("Double_attr");
            attr_list.push_back("Float_attr");
            attr_list.push_back("Long_attr");
            attr_list.push_back("Long64_attr");
            attr_list.push_back("Short_attr");
            attr_list.push_back("UChar_attr");
            attr_list.push_back("ULong_attr");
            attr_list.push_back("ULong64_attr");
            attr_list.push_back("UShort_attr");
            init_attr_conf = device1->get_attribute_config_ex(attr_list);

            def_timeout = device1->get_timeout_millis();
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            exit(-1);
        }
    }

    virtual ~SUITE_NAME()
    {
        time_t ti = Tango::get_current_system_datetime();
        TEST_LOG << "Destroying suite at " << ctime(&ti) << endl;
        DevLong lg;
        DeviceData din;
        lg = 1246;
        din << lg;
        try
        {
            device1->set_timeout_millis(9000);
            device1->set_attribute_config(*init_attr_conf);

            device1->command_inout("IOSetAttr", din);
            din << device1_name;
            dserver->command_inout("DevRestart", din);
            device1->set_timeout_millis(def_timeout);
        }
        catch(CORBA::Exception &e)
        {
            TEST_LOG << endl << "Exception in suite tearDown():" << endl;
            Except::print_exception(e);
            exit(-1);
        }

        //
        // Clean up --------------------------------------------------------
        //

        // clean up in case test suite terminates before timeout is restored to defaults
        if(CxxTest::TangoPrinter::is_restore_set("timeout"))
        {
            try
            {
                device1->set_timeout_millis(def_timeout);
            }
            catch(DevFailed &e)
            {
                TEST_LOG << endl << "Exception in suite tearDown():" << endl;
                Except::print_exception(e);
            }
        }

        delete device1;
        delete dserver;
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
    // Test set/get min/max alarm/warning functions
    //

    void test_set_get_alarms(void)
    {
        const DevVarStringArray *alarms;
        DeviceData dout;

        device1->set_timeout_millis(10 * def_timeout);
        CxxTest::TangoPrinter::restore_set("timeout");

        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("SetGetAlarms"));
        TS_ASSERT_THROWS_NOTHING(dout >> alarms);

        TS_ASSERT_EQUALS((*alarms).length(), 45u);
        TS_ASSERT_EQUALS(string((*alarms)[0].in()), "Double_attr");
        TS_ASSERT_EQUALS(string((*alarms)[1].in()), "-999.99");
        TS_ASSERT_EQUALS(string((*alarms)[2].in()), "-888.88");
        TS_ASSERT_EQUALS(string((*alarms)[3].in()), "888.88");
        TS_ASSERT_EQUALS(string((*alarms)[4].in()), "999.99");
        TS_ASSERT_EQUALS(string((*alarms)[5].in()), "Float_attr");
        TS_ASSERT_EQUALS(string((*alarms)[6].in()), "-777.77");
        TS_ASSERT_EQUALS(string((*alarms)[7].in()), "-666.66");
        TS_ASSERT_EQUALS(string((*alarms)[8].in()), "666.66");
        TS_ASSERT_EQUALS(string((*alarms)[9].in()), "777.77");
        TS_ASSERT_EQUALS(string((*alarms)[10].in()), "Long_attr");
        TS_ASSERT_EQUALS(string((*alarms)[11].in()), "1000");
        TS_ASSERT_EQUALS(string((*alarms)[12].in()), "1100");
        TS_ASSERT_EQUALS(string((*alarms)[13].in()), "1400");
        TS_ASSERT_EQUALS(string((*alarms)[14].in()), "1500");
        TS_ASSERT_EQUALS(string((*alarms)[15].in()), "Long64_attr");
        TS_ASSERT_EQUALS(string((*alarms)[16].in()), "-90000");
        TS_ASSERT_EQUALS(string((*alarms)[17].in()), "-80000");
        TS_ASSERT_EQUALS(string((*alarms)[18].in()), "80000");
        TS_ASSERT_EQUALS(string((*alarms)[19].in()), "90000");
        TS_ASSERT_EQUALS(string((*alarms)[20].in()), "Short_attr");
        TS_ASSERT_EQUALS(string((*alarms)[21].in()), "-5000");
        TS_ASSERT_EQUALS(string((*alarms)[22].in()), "-4000");
        TS_ASSERT_EQUALS(string((*alarms)[23].in()), "4000");
        TS_ASSERT_EQUALS(string((*alarms)[24].in()), "5000");
        TS_ASSERT_EQUALS(string((*alarms)[25].in()), "UChar_attr");
        TS_ASSERT_EQUALS(string((*alarms)[26].in()), "1");
        TS_ASSERT_EQUALS(string((*alarms)[27].in()), "2");
        TS_ASSERT_EQUALS(string((*alarms)[28].in()), "230");
        TS_ASSERT_EQUALS(string((*alarms)[29].in()), "240");
        TS_ASSERT_EQUALS(string((*alarms)[30].in()), "ULong_attr");
        TS_ASSERT_EQUALS(string((*alarms)[31].in()), "1");
        TS_ASSERT_EQUALS(string((*alarms)[32].in()), "2");
        TS_ASSERT_EQUALS(string((*alarms)[33].in()), "666666");
        TS_ASSERT_EQUALS(string((*alarms)[34].in()), "777777");
        TS_ASSERT_EQUALS(string((*alarms)[35].in()), "ULong64_attr");
        TS_ASSERT_EQUALS(string((*alarms)[36].in()), "1");
        TS_ASSERT_EQUALS(string((*alarms)[37].in()), "2");
        TS_ASSERT_EQUALS(string((*alarms)[38].in()), "77777777");
        TS_ASSERT_EQUALS(string((*alarms)[39].in()), "88888888");
        TS_ASSERT_EQUALS(string((*alarms)[40].in()), "UShort_attr");
        TS_ASSERT_EQUALS(string((*alarms)[41].in()), "1");
        TS_ASSERT_EQUALS(string((*alarms)[42].in()), "2");
        TS_ASSERT_EQUALS(string((*alarms)[43].in()), "20000");
        TS_ASSERT_EQUALS(string((*alarms)[44].in()), "30000");

        device1->set_timeout_millis(def_timeout);
        CxxTest::TangoPrinter::restore_unset("timeout");
    }

    //
    // Test set/get min/max value functions
    //

    void test_set_get_ranges(void)
    {
        const DevVarStringArray *ranges;
        DeviceData dout;

        device1->set_timeout_millis(6 * def_timeout);
        CxxTest::TangoPrinter::restore_set("timeout");

        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("SetGetRanges"));
        TS_ASSERT_THROWS_NOTHING(dout >> ranges);

        TS_ASSERT_EQUALS((*ranges).length(), 27u);
        TS_ASSERT_EQUALS(string((*ranges)[0].in()), "Double_attr_w");
        TS_ASSERT_EQUALS(string((*ranges)[1].in()), "-1111.11");
        TS_ASSERT_EQUALS(string((*ranges)[2].in()), "1111.11");
        TS_ASSERT_EQUALS(string((*ranges)[3].in()), "Float_attr_w");
        TS_ASSERT_EQUALS(string((*ranges)[4].in()), "-888.88");
        TS_ASSERT_EQUALS(string((*ranges)[5].in()), "888.88");
        TS_ASSERT_EQUALS(string((*ranges)[6].in()), "Long_attr_w");
        TS_ASSERT_EQUALS(string((*ranges)[7].in()), "900");
        TS_ASSERT_EQUALS(string((*ranges)[8].in()), "1600");
        TS_ASSERT_EQUALS(string((*ranges)[9].in()), "Long64_attr_rw");
        TS_ASSERT_EQUALS(string((*ranges)[10].in()), "-100000");
        TS_ASSERT_EQUALS(string((*ranges)[11].in()), "100000");
        TS_ASSERT_EQUALS(string((*ranges)[12].in()), "Short_attr_w");
        TS_ASSERT_EQUALS(string((*ranges)[13].in()), "-6000");
        TS_ASSERT_EQUALS(string((*ranges)[14].in()), "6000");
        TS_ASSERT_EQUALS(string((*ranges)[15].in()), "UChar_attr_w");
        TS_ASSERT_EQUALS(string((*ranges)[16].in()), "0");
        TS_ASSERT_EQUALS(string((*ranges)[17].in()), "250");
        TS_ASSERT_EQUALS(string((*ranges)[18].in()), "ULong_attr_rw");
        TS_ASSERT_EQUALS(string((*ranges)[19].in()), "0");
        TS_ASSERT_EQUALS(string((*ranges)[20].in()), "888888");
        TS_ASSERT_EQUALS(string((*ranges)[21].in()), "ULong64_attr_rw");
        TS_ASSERT_EQUALS(string((*ranges)[22].in()), "0");
        TS_ASSERT_EQUALS(string((*ranges)[23].in()), "99999999");
        TS_ASSERT_EQUALS(string((*ranges)[24].in()), "UShort_attr_w");
        TS_ASSERT_EQUALS(string((*ranges)[25].in()), "0");
        TS_ASSERT_EQUALS(string((*ranges)[26].in()), "40000");

        device1->set_timeout_millis(def_timeout);
        CxxTest::TangoPrinter::restore_unset("timeout");
    }

    //
    // Test set/get properties functions
    //

    void test_set_get_properties(void)
    {
        /*        const DevVarStringArray *props;
                DeviceData dout;

                device1->set_timeout_millis(25*def_timeout);
                CxxTest::TangoPrinter::restore_set("timeout");

        time_t ti = Tango::get_current_system_datetime();
        TEST_LOG << "Calling SetGetProperties at " << ctime(&ti) << endl;
        struct timeval start,stop;
        gettimeofday(&start,NULL);
                TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("SetGetProperties"));
                TS_ASSERT_THROWS_NOTHING(dout >> props);
        gettimeofday(&stop,NULL);
        double elapsed = (double)(stop.tv_sec - start.tv_sec) + (double)(stop.tv_usec - start.tv_usec) / 1000000.0;
        TEST_LOG << "required time for command SetGetProperties = " << elapsed << endl;

        //        TEST_LOG << "## prop length = " << (*props).length() << endl;

                TS_ASSERT_EQUALS((*props).length(), 420);
                size_t i = 0;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Double_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90"); i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "Double_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;





                TS_ASSERT_EQUALS(string((*props)[i].in()), "Float_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90"); i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "Float_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;







                TS_ASSERT_EQUALS(string((*props)[i].in()), "Long_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90"); i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "Long_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;




                TS_ASSERT_EQUALS(string((*props)[i].in()), "Long64_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90"); i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "Long64_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;






                TS_ASSERT_EQUALS(string((*props)[i].in()), "Short_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90"); i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "Short_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;





                TS_ASSERT_EQUALS(string((*props)[i].in()), "UChar_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90"); i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "UChar_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;




                TS_ASSERT_EQUALS(string((*props)[i].in()), "ULong_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90"); i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "ULong_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;




                TS_ASSERT_EQUALS(string((*props)[i].in()), "ULong64_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90"); i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "ULong64_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;



                TS_ASSERT_EQUALS(string((*props)[i].in()), "UShort_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90");i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "UShort_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;


                TS_ASSERT_EQUALS(string((*props)[i].in()), "Encoded_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "200"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "190"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "20"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "180"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "5"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "10"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "300"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "400"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.2,0.3"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "40,50"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.6,0.7"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "80,90");i++;

                TS_ASSERT_EQUALS(string((*props)[i].in()), "Encoded_attr"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_label"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_description"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_standard_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_display_unit"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "Test_format"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "1"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "201"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "191"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "21"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "181"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "6"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "11"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "301"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "401"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.3,0.4"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "41,51"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "0.7,0.8"); i++;
                TS_ASSERT_EQUALS(string((*props)[i].in()), "81,91"); i++;


                device1->set_timeout_millis(def_timeout);
                CxxTest::TangoPrinter::restore_unset("timeout");*/
    }

    // Test read attribute exceptions

    void test_read_attribute_exceptions(void)
    {
        DeviceAttribute attr;
        DevShort sh;
        DevLong lg;

        TS_ASSERT_THROWS_NOTHING(attr = device1->read_attribute("Toto"));
        TS_ASSERT_THROWS_ASSERT(
            attr >> sh, Tango::DevFailed & e, TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrNotFound);
            TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));

        TS_ASSERT_THROWS_NOTHING(attr = device1->read_attribute("attr_no_data"));
        TS_ASSERT_THROWS_ASSERT(
            attr >> sh, Tango::DevFailed & e, TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrValueNotSet);
            TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));

        TS_ASSERT_THROWS_NOTHING(attr = device1->read_attribute("attr_wrong_type"));
        TS_ASSERT_THROWS_ASSERT(
            attr >> sh, Tango::DevFailed & e, TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrOptProp);
            TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));

        TS_ASSERT_THROWS_NOTHING(attr = device1->read_attribute("attr_wrong_size"));
        TS_ASSERT_THROWS_ASSERT(
            attr >> lg, Tango::DevFailed & e, TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrOptProp);
            TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));

        TS_ASSERT_THROWS_NOTHING(attr = device1->read_attribute("attr_no_alarm"));
        TS_ASSERT_THROWS_ASSERT(
            attr >> lg, Tango::DevFailed & e, TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), API_AttrNoAlarm);
            TS_ASSERT_EQUALS(e.errors[0].severity, Tango::ERR));
    }

    //
    // Testing SCALAR attribute of type different than READ
    // As we have never written any attribute (yet), the write values should all
    // be initialised to 0
    //

    void test_SCALAR_attribute_of_type_different_than_READ(void)
    {
        DeviceAttribute long_attr_with_w, long_attr_w, short_attr_rw, float_attr_w, ushort_attr_w, uchar_attr_w;
        DevVarLongArray *lg_array;
        DevVarShortArray *sh_array;
        DevVarFloatArray *fl_array;
        DevVarUShortArray *ush_array;
        DevVarUCharArray *uch_array;

        TS_ASSERT_THROWS_NOTHING(long_attr_with_w = device1->read_attribute("Long_attr_with_w"));
        TS_ASSERT_EQUALS(long_attr_with_w.get_name(), "Long_attr_with_w");
        TS_ASSERT_EQUALS(long_attr_with_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(long_attr_with_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(long_attr_with_w.get_dim_y(), 0);
        long_attr_with_w >> lg_array;
        TS_ASSERT_EQUALS((*lg_array)[0], 1246);
        TS_ASSERT_EQUALS((*lg_array)[1], 0);

        TS_ASSERT_THROWS_NOTHING(long_attr_w = device1->read_attribute("Long_attr_w"));
        TS_ASSERT_EQUALS(long_attr_w.get_name(), "Long_attr_w");
        TS_ASSERT_EQUALS(long_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(long_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(long_attr_w.get_dim_y(), 0);
        long_attr_w >> lg_array;
        TS_ASSERT_EQUALS((*lg_array)[0], 0);

        TS_ASSERT_THROWS_NOTHING(short_attr_rw = device1->read_attribute("Short_attr_rw"));
        TS_ASSERT_EQUALS(short_attr_rw.get_name(), "Short_attr_rw");
        TS_ASSERT_EQUALS(short_attr_rw.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(short_attr_rw.get_dim_x(), 1);
        TS_ASSERT_EQUALS(short_attr_rw.get_dim_y(), 0);
        short_attr_rw >> sh_array;
        TS_ASSERT_EQUALS((*sh_array)[0], 66);
        TS_ASSERT_EQUALS((*sh_array)[1], 0);

        TS_ASSERT_THROWS_NOTHING(float_attr_w = device1->read_attribute("Float_attr_w"));
        TS_ASSERT_EQUALS(float_attr_w.get_name(), "Float_attr_w");
        TS_ASSERT_EQUALS(float_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(float_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(float_attr_w.get_dim_y(), 0);
        float_attr_w >> fl_array;
        TS_ASSERT_EQUALS((*fl_array)[0], 0);

        TS_ASSERT_THROWS_NOTHING(ushort_attr_w = device1->read_attribute("UShort_attr_w"));
        TS_ASSERT_EQUALS(ushort_attr_w.get_name(), "UShort_attr_w");
        TS_ASSERT_EQUALS(ushort_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(ushort_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(ushort_attr_w.get_dim_y(), 0);
        ushort_attr_w >> ush_array;
        TS_ASSERT_EQUALS((*ush_array)[0], 0);

        TS_ASSERT_THROWS_NOTHING(uchar_attr_w = device1->read_attribute("UChar_attr_w"));
        TS_ASSERT_EQUALS(uchar_attr_w.get_name(), "UChar_attr_w");
        TS_ASSERT_EQUALS(uchar_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(uchar_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(uchar_attr_w.get_dim_y(), 0);
        uchar_attr_w >> uch_array;
        TS_ASSERT_EQUALS((*uch_array)[0], 0);
    }

    // Test read attribute on write type attribute

    void test_read_attribute_on_write_type_attribute(void)
    {
        DeviceAttribute short_attr_w2, long_attr_w, double_attr_w, string_attr_w2;
        DevVarShortArray *sh_array;
        DevVarLongArray *lg_array;
        DevVarDoubleArray *db_array;
        DevVarStringArray *str_array;

        TS_ASSERT_THROWS_NOTHING(short_attr_w2 = device1->read_attribute("Short_attr_w2"));
        TS_ASSERT_EQUALS(short_attr_w2.get_name(), "Short_attr_w2");
        TS_ASSERT_EQUALS(short_attr_w2.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(short_attr_w2.get_dim_x(), 1);
        TS_ASSERT_EQUALS(short_attr_w2.get_dim_y(), 0);
        short_attr_w2 >> sh_array;
        TS_ASSERT_EQUALS((*sh_array)[0], 0);

        TS_ASSERT_THROWS_NOTHING(long_attr_w = device1->read_attribute("Long_attr_w"));
        TS_ASSERT_EQUALS(long_attr_w.get_name(), "Long_attr_w");
        TS_ASSERT_EQUALS(long_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(long_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(long_attr_w.get_dim_y(), 0);
        long_attr_w >> lg_array;
        TS_ASSERT_EQUALS((*lg_array)[0], 0);

        TS_ASSERT_THROWS_NOTHING(double_attr_w = device1->read_attribute("Double_attr_w"));
        TS_ASSERT_EQUALS(double_attr_w.get_name(), "Double_attr_w");
        TS_ASSERT_EQUALS(double_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(double_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(double_attr_w.get_dim_y(), 0);
        double_attr_w >> db_array;
        TS_ASSERT_EQUALS((*db_array)[0], 0);

        TS_ASSERT_THROWS_NOTHING(string_attr_w2 = device1->read_attribute("String_attr_w2"));
        TS_ASSERT_EQUALS(string_attr_w2.get_name(), "String_attr_w2");
        TS_ASSERT_EQUALS(string_attr_w2.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(string_attr_w2.get_dim_x(), 1);
        TS_ASSERT_EQUALS(string_attr_w2.get_dim_y(), 0);
        string_attr_w2 >> str_array;
        TS_ASSERT_EQUALS(string((*str_array)[0].in()), "Not initialised");
    }

    void test_write_attribute_error_message(void)
    {
        DeviceData di;

        vector<short> data_in(2);
        data_in[0] = 6;
        data_in[1] = 1;
        di << data_in;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOAttrThrowEx", di));

        TS_ASSERT_THROWS_ASSERT(
            device1->command_inout("IOInitWAttr"),
            Tango::DevFailed & e,
            TS_ASSERT(string(e.errors[0].reason.in()) == "API_IncompatibleAttrDataType" &&
                      string(e.errors[0].desc.in()).find("expected Tango::DevVarShortArray") != string::npos &&
                      string(e.errors[0].desc.in()).find("found Tango::DevVarUShortArray") != string::npos &&
                      e.errors[0].severity == Tango::ERR));

        data_in[0] = 6;
        data_in[1] = 0;
        di << data_in;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOAttrThrowEx", di));
    }

    // Test read attribute on initialised write type attribute

    void test_read_attribute_on_initialised_write_type_attribute(void)
    {
        DeviceAttribute short_attr_w, long_attr_w, double_attr_w, string_attr_w;
        DevVarShortArray *sh_array;
        DevVarLongArray *lg_array;
        DevVarDoubleArray *db_array;
        DevVarStringArray *str_array;

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOInitWAttr"));

        TS_ASSERT_THROWS_NOTHING(short_attr_w = device1->read_attribute("Short_attr_w"));
        TS_ASSERT_EQUALS(short_attr_w.get_name(), "Short_attr_w");
        TS_ASSERT_EQUALS(short_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(short_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(short_attr_w.get_dim_y(), 0);
        short_attr_w >> sh_array;
        TS_ASSERT_EQUALS((*sh_array)[0], 10);

        TS_ASSERT_THROWS_NOTHING(long_attr_w = device1->read_attribute("Long_attr_w"));
        TS_ASSERT_EQUALS(long_attr_w.get_name(), "Long_attr_w");
        TS_ASSERT_EQUALS(long_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(long_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(long_attr_w.get_dim_y(), 0);
        long_attr_w >> lg_array;
        TS_ASSERT_EQUALS((*lg_array)[0], 100);

        TS_ASSERT_THROWS_NOTHING(double_attr_w = device1->read_attribute("Double_attr_w"));
        TS_ASSERT_EQUALS(double_attr_w.get_name(), "Double_attr_w");
        TS_ASSERT_EQUALS(double_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(double_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(double_attr_w.get_dim_y(), 0);
        double_attr_w >> db_array;
        TS_ASSERT_EQUALS((*db_array)[0], 1.1);

        TS_ASSERT_THROWS_NOTHING(string_attr_w = device1->read_attribute("String_attr_w"));
        TS_ASSERT_EQUALS(string_attr_w.get_name(), "String_attr_w");
        TS_ASSERT_EQUALS(string_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(string_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(string_attr_w.get_dim_y(), 0);
        string_attr_w >> str_array;
        TS_ASSERT_EQUALS(string((*str_array)[0].in()), "Init");
    }

    // Test read attribute on initialised read/write type attribute

    void test_read_attribute_on_initialised_read_write_type_attribute(void)
    {
        DeviceAttribute state_attr_w;
        DevVarStateArray *state_array;
        std::vector<Tango::DevState> state_vector;

        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOInitRWAttr"));

        TS_ASSERT_THROWS_NOTHING(state_attr_w = device1->read_attribute("State_attr_rw"));
        TS_ASSERT_EQUALS(state_attr_w.get_name(), "State_attr_rw");
        TS_ASSERT_EQUALS(state_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(state_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(state_attr_w.get_dim_y(), 0);
        TS_ASSERT_EQUALS(state_attr_w.get_written_dim_x(), 1);
        TS_ASSERT_EQUALS(state_attr_w.get_written_dim_y(), 0);
        state_attr_w >> state_array;
        TS_ASSERT_EQUALS((*state_array)[1], Tango::UNKNOWN);
        delete state_array;

        // Test the extraction in a vector of States
        TS_ASSERT_THROWS_NOTHING(state_attr_w = device1->read_attribute("State_attr_rw"));
        TS_ASSERT_EQUALS(state_attr_w.get_name(), "State_attr_rw");
        TS_ASSERT_EQUALS(state_attr_w.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(state_attr_w.get_dim_x(), 1);
        TS_ASSERT_EQUALS(state_attr_w.get_dim_y(), 0);
        TS_ASSERT_EQUALS(state_attr_w.get_written_dim_x(), 1);
        TS_ASSERT_EQUALS(state_attr_w.get_written_dim_y(), 0);
        state_attr_w >> state_vector;
        TS_ASSERT_EQUALS(state_vector[1], Tango::UNKNOWN);
    }

    //
    // Test alarm on attribute. An alarm is defined for the Long_attr attribute
    // is < 1000 and > 1500.
    //

    void test_alarm_on_attribute(void)
    {
        DeviceAttribute long_attr;
        DevVarLongArray *lg_array;
        DevLong lg;
        DeviceData din, dout;
        DevState state;
        const char *status;

        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("State"));
        dout >> state;
        TS_ASSERT_EQUALS(state, Tango::ON);

        TS_ASSERT_THROWS_NOTHING(long_attr = device1->read_attribute("Long_attr"));
        TS_ASSERT_EQUALS(long_attr.get_name(), "Long_attr");
        TS_ASSERT_EQUALS(long_attr.get_quality(), Tango::ATTR_VALID);
        TS_ASSERT_EQUALS(long_attr.get_dim_x(), 1);
        TS_ASSERT_EQUALS(long_attr.get_dim_y(), 0);
        long_attr >> lg_array;
        TS_ASSERT_EQUALS((*lg_array)[0], 1246);

        lg = 900;
        din << lg;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOSetAttr", din));
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("State"));
        dout >> state;
        TS_ASSERT_EQUALS(state, Tango::ALARM);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("Status"));
        dout >> status;
        TEST_LOG << "status = " << status << endl;
        TS_ASSERT_EQUALS(std::string(status), "The device is in ALARM state.\nAlarm : Value too low for Long_attr");

        lg = 1200;
        din << lg;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOSetAttr", din));
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("State"));
        dout >> state;
        TS_ASSERT_EQUALS(state, Tango::ON);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("Status"));
        dout >> status;
        TS_ASSERT_EQUALS(std::string(status), "The device is in ON state.");

        state = Tango::ON;
        din << state;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOState", din));
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("State"));
        dout >> state;
        TS_ASSERT_EQUALS(state, Tango::ON);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("Status"));
        dout >> status;
        TS_ASSERT_EQUALS(std::string(status), "The device is in ON state.");

        lg = 2000;
        din << lg;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOSetAttr", din));
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("State"));
        dout >> state;
        TS_ASSERT_EQUALS(state, Tango::ALARM);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("Status"));
        dout >> status;
        TS_ASSERT_EQUALS(std::string(status), "The device is in ALARM state.\nAlarm : Value too high for Long_attr");

        lg = 1200;
        din << lg;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOSetAttr", din));
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("State"));
        dout >> state;
        TS_ASSERT_EQUALS(state, Tango::ON);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("Status"));
        dout >> status;
        TS_ASSERT_EQUALS(std::string(status), "The device is in ON state.");

        state = Tango::ON;
        din << state;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOState", din));
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("State"));
        dout >> state;
        TS_ASSERT_EQUALS(state, Tango::ON);
        TS_ASSERT_THROWS_NOTHING(dout = device1->command_inout("Status"));
        dout >> status;
        TS_ASSERT_EQUALS(std::string(status), "The device is in ON state.");
    }

    void set_Long_attr_value(DevLong value)
    {
        DeviceData input;
        input << value;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOSetAttr", input));
    }

    void set_attribute_exception_flag(short attribute_disc, bool enabled)
    {
        std::vector<short> flags(2);
        flags[0] = attribute_disc;
        flags[1] = enabled ? 1 : 0;
        DeviceData data;
        data << flags;
        TS_ASSERT_THROWS_NOTHING(device1->command_inout("IOAttrThrowEx", data));
    }

    void __assert_dev_state(DevState expected, const char *location)
    {
        std::string message = std::string("Called from ") + location;

        DevState state;
        DeviceData data;
        TSM_ASSERT_THROWS_NOTHING(message, data = device1->command_inout("State"));
        data >> state;
        TSM_ASSERT_EQUALS(message, expected, state);
    }

#define assert_dev_state(expected) __assert_dev_state(expected, TANGO_FILE_AND_LINE)

    // Verifies that device state is set correctly when alarm is configured for an attribute
    // but no value is provided for this attribute in user callback (e.g. an exception is thrown).

    void test_alarm_on_attribute_exception_during_read(void)
    {
        const short EXCEPTION_IN_Long_attr = 5;

        set_Long_attr_value(2000);
        assert_dev_state(Tango::ALARM);

        set_attribute_exception_flag(EXCEPTION_IN_Long_attr, true);
        assert_dev_state(Tango::ON);

        set_attribute_exception_flag(EXCEPTION_IN_Long_attr, false);
        assert_dev_state(Tango::ALARM);

        set_Long_attr_value(1200);
        assert_dev_state(Tango::ON);
    }

#undef assert_dev_state

    /*
     * Test for changing alarm treshold to value lower than currently read from
     * hardware. Attribute should have alarm quality after property change.
     */

    void test_change_max_alarm_threshold_below_current_value()
    {
        const char *attr_name = "Short_attr_rw";
        const DevShort attr_value = 20;

        DeviceAttribute value(attr_name, attr_value);
        TS_ASSERT_THROWS_NOTHING(device1->write_attribute(value));

        TS_ASSERT_EQUALS(Tango::ON, device1->state());
        TS_ASSERT_EQUALS(Tango::ATTR_VALID, device1->read_attribute(attr_name).get_quality());

        auto config = device1->get_attribute_config(attr_name);
        config.alarms.max_alarm = std::to_string(attr_value - 1);
        AttributeInfoListEx config_in = {config};
        TS_ASSERT_THROWS_NOTHING(device1->set_attribute_config(config_in));

        TS_ASSERT_EQUALS(Tango::ALARM, device1->state());
        TS_ASSERT_EQUALS(Tango::ATTR_ALARM, device1->read_attribute(attr_name).get_quality());
    }

    /*
     * Tests for reading multiple attributes in a single network call where the last
     * attribute has a valid quality but the read callback does not set any value.
     *
     * For Device IDLv3 (or better) no value is returned from the server for such
     * attributes and API_AttrValueNotSet exception is thrown if value extraction
     * is attempted but other attributes are not affected.
     *
     * For v1 and v2 the whole call fails with API_AttrValueNotSet exception.
     * Previously this scenario resulted in a crash due to double-delete problem.
     */

    void test_multiple_attributes_read_in_one_call_last_has_no_data_dev_impl_3()
    {
        Tango::DevVarStringArray attribute_names;
        std::vector<std::string> attribute_names_value = {"Long_attr", "attr_no_data"};
        attribute_names << attribute_names_value;

        std::unique_ptr<std::vector<Tango::DeviceAttribute>> result;

        TS_ASSERT_THROWS_NOTHING(result = std::unique_ptr<std::vector<Tango::DeviceAttribute>>(
                                     device1->read_attributes(attribute_names_value)));

        TS_ASSERT_EQUALS(2u, result->size());

        Tango::DevLong long_value = 0;
        Tango::DevShort short_value = 0;

        TS_ASSERT_THROWS_NOTHING((*result)[0] >> long_value);

        TS_ASSERT_THROWS_ASSERT((*result)[1] >> short_value,
                                Tango::DevFailed & e,
                                TS_ASSERT(std::string(e.errors[0].reason.in()) == API_AttrValueNotSet));
    }

    void test_multiple_attributes_read_in_one_call_last_has_no_data_dev_impl_1_2()
    {
        Tango::DevVarStringArray attribute_names;
        std::vector<std::string> attribute_names_value = {"Long_attr", "attr_no_data"};
        attribute_names << attribute_names_value;

        AttributeValueList_var result;

        // Note that we are using get_device() to access raw CORBA stub and
        // explicitly call DeviceImpl::read_attributes. This is required to
        // bypass dispatching logic in DeviceProxy and force use of IDLv1/v2.
        TS_ASSERT_THROWS_ASSERT(result = device1->get_device()->read_attributes(attribute_names),
                                Tango::DevFailed & e,
                                TS_ASSERT(std::string(e.errors[0].reason.in()) == API_AttrValueNotSet));
    }
};

// NOLINTEND(*)
