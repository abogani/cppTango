// NOLINTBEGIN(*)

#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME DevIntrNotRunningTest

class EventCallBack : public CountingCallBack<Tango::DevIntrChangeEventData>
{
  public:
    std::string cb_err_reason;

  private:
    bool process_event(Tango::DevIntrChangeEventData *event) override
    {
        TS_ASSERT_EQUALS(event->errors.length(), 1u);
        cb_err_reason = event->errors[0].reason;

        return event->err;
    }
};

class DevIntrNotRunningTest : public CxxTest::TestSuite
{
  public:
    SUITE_NAME()
    {
        CxxTest::TangoPrinter::validate_args();
    }

    virtual ~SUITE_NAME() { }

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
    void test_not_running_error()
    {
        try
        {
            // connect to a defined device which is not running
            auto device = std::make_shared<DeviceProxy>("sys/tg_test/1");

            EventCallBack cb{};

            device->subscribe_event(Tango::INTERFACE_CHANGE_EVENT, &cb, true);

            TS_ASSERT_EQUALS(cb.invocation_count(), 1);
            TS_ASSERT_EQUALS(cb.error_count(), 1);
            TS_ASSERT_EQUALS(cb.cb_err_reason, std::string(API_CantConnectToDevice));
        }
        catch(Tango::DevFailed &e)
        {
            Except::print_exception(e);
            TS_FAIL("Unexpected exception");
        }
        catch(CORBA::Exception &e)
        {
            Except::print_exception(e);
            TS_FAIL("Unexpected exception");
        }
    }
};

// NOLINTEND(*)
