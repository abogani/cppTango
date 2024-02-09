// NOLINTBEGIN(*)

#ifndef DynamicAttributesTestSuite_h
  #define DynamicAttributesTestSuite_h

  #include "cxx_common.h"

  #undef SUITE_NAME
  #define SUITE_NAME DynamicAttributesTestSuite

class DynamicAttributesTestSuite : public CxxTest::TestSuite
{
  protected:
    DeviceProxy *device1, *dserver;
    std::string device1_name;

  public:
    SUITE_NAME()
    {
        //
        // Arguments check -------------------------------------------------
        //

        std::string dserver_name;

        // predefined mandatory parameters
        device1_name = CxxTest::TangoPrinter::get_param("device1");
        dserver_name = "dserver/" + CxxTest::TangoPrinter::get_param("fulldsname");

        // predefined optional parameters
        CxxTest::TangoPrinter::get_param_opt("loop"); // loop parameter is then managed by the CXX framework itself

        // always add this line, otherwise arguments will not be parsed correctly
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

        // clean up in case test suite terminates before my_restore_point is restored to defaults
        if(CxxTest::TangoPrinter::is_restore_set("my_restore_point"))
        {
            try
            {
                // execute some instructions
            }
            catch(DevFailed &e)
            {
                TEST_LOG << endl << "Exception in suite tearDown():" << endl;
                Except::print_exception(e);
            }
        }

        // delete dynamically allocated objects
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

    // Test cppTango#1022

    void test_CppTangoIssue1022()
    {
        try
        {
            Tango::DeviceAttribute da = device1->read_attribute("Attr1");
            Tango::DevDouble double_val = -1.0;
            da >> double_val;
            TS_ASSERT_EQUALS(double_val, 0.0);
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            TS_ASSERT(false);
        }

        try
        {
            Tango::DeviceData din;
            din << device1_name;
            dserver->command_inout("DevRestart", din);
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            TS_ASSERT(false);
        }

        try
        {
            Tango::DeviceAttribute da = device1->read_attribute("Attr1");
            Tango::DevDouble double_val = -1.0;
            da >> double_val;
            TS_ASSERT_EQUALS(double_val, 0.0);
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            TS_ASSERT(false);
        }

        try
        {
            Tango::DeviceData din;
            din << device1_name;
            dserver->command_inout("DevRestart", din);
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            TS_ASSERT(false);
        }

        try
        {
            Tango::DeviceAttribute da = device1->read_attribute("Attr1");
            Tango::DevDouble double_val = -1.0;
            da >> double_val;
            TS_ASSERT_EQUALS(double_val, 0.0);
        }
        catch(const Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            TS_ASSERT(false);
        }
    }
};

#endif // DynamicAttributesTestSuite_h

// NOLINTEND(*)
