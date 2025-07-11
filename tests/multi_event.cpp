// NOLINTBEGIN(*)

#include "old_common.h"

class EventCallBack : public Tango::CallBack
{
    void push_event(Tango::EventData *);

  public:
    int cb_executed;
    int cb_err;
    int old_sec, old_usec;
    long val;
    long val_size;
};

void EventCallBack::push_event(Tango::EventData *event_data)
{
    std::vector<DevLong> value;
    struct timeval now_timeval = Tango::make_timeval(std::chrono::system_clock::now());

    TEST_LOG << "date : tv_sec = " << now_timeval.tv_sec;
    TEST_LOG << ", tv_usec = " << now_timeval.tv_usec << std::endl;

    auto delta_msec = ((now_timeval.tv_sec - old_sec) * 1000) + ((now_timeval.tv_usec - old_usec) / 1000);

    old_sec = now_timeval.tv_sec;
    old_usec = now_timeval.tv_usec;

    TEST_LOG << "delta_msec = " << delta_msec << std::endl;

    cb_executed++;

    try
    {
        TEST_LOG << "StateEventCallBack::push_event(): called attribute " << event_data->attr_name << " event "
                 << event_data->event << "\n";
        if(!event_data->err)
        {
            *(event_data->attr_value) >> value;
            TEST_LOG << "CallBack value size " << value.size() << std::endl;
            val = value[2];
            val_size = value.size();
            TEST_LOG << "Callback value " << val << std::endl;
        }
        else
        {
            TEST_LOG << "Error send to callback" << std::endl;
            //            Tango::Except::print_error_stack(event_data->errors);
            if(strcmp(event_data->errors[0].reason.in(), "bbb") == 0)
            {
                cb_err++;
            }
        }
    }
    catch(...)
    {
        TEST_LOG << "EventCallBack::push_event(): could not extract data !\n";
    }
}

//-----------------------------------------------------------------------------------

class EventUnsubCallBack : public Tango::CallBack
{
  public:
    EventUnsubCallBack(DeviceProxy *d) :
        Tango::CallBack(),
        cb_executed(0),
        dev(d)
    {
    }

    void push_event(Tango::EventData *);

    void set_ev_id(int e)
    {
        ev_id = e;
    }

  protected:
    int cb_executed;
    int ev_id;
    Tango::DeviceProxy *dev;
};

void EventUnsubCallBack::push_event(Tango::EventData *event_data)
{
    TEST_LOG << "Event received for attribute " << event_data->attr_name << std::endl;
    cb_executed++;
    if(cb_executed == 2)
    {
        dev->unsubscribe_event(ev_id);
    }
}

//---------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if(argc == 1)
    {
        TEST_LOG << "usage: %s device" << std::endl;
        exit(-1);
    }

    std::string device_name = argv[1];

    try
    {
        device = new DeviceProxy(device_name);
    }
    catch(CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    TEST_LOG << std::endl << "new DeviceProxy(" << device->name() << ") returned" << std::endl << std::endl;

    try
    {
        std::string att_name("Event_change_tst");

        //
        // Test set up (stop polling and clear abs_change and rel_change attribute
        // properties but restart device to take this into account)
        // Set the abs_change to 1
        //

        if(device->is_attribute_polled(att_name))
        {
            device->stop_poll_attribute(att_name);
        }
        DbAttribute dba(att_name, device_name);
        DbData dbd;
        DbDatum a(att_name);
        a << (short) 2;
        dbd.push_back(a);
        dbd.push_back(DbDatum("abs_change"));
        dbd.push_back(DbDatum("rel_change"));
        dba.delete_property(dbd);

        dbd.clear();
        a << (short) 1;
        dbd.push_back(a);
        DbDatum ch("abs_change");
        ch << (short) 1;
        dbd.push_back(ch);
        dba.put_property(dbd);

        DeviceProxy adm_dev(device->adm_name().c_str());
        DeviceData di;
        di << device_name;
        adm_dev.command_inout("DevRestart", di);

        delete device;
        device = new DeviceProxy(device_name);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        //
        // subscribe 2 times to the same change event
        //

        int eve_id1, eve_id2;
        std::vector<std::string> filters;
        EventCallBack cb;
        cb.cb_executed = 0;
        cb.cb_err = 0;
        cb.old_sec = cb.old_usec = 0;

        // switch on the polling first!
        device->poll_attribute(att_name, 1000);

        eve_id1 = device->subscribe_event(att_name, Tango::CHANGE_EVENT, &cb, filters);
        eve_id2 = device->subscribe_event(att_name, Tango::CHANGE_EVENT, &cb, filters);

        //
        // Check that the attribute is now polled at 1000 mS
        //

        bool po = device->is_attribute_polled(att_name);
        TEST_LOG << "attribute polled : " << po << std::endl;
        assert(po == true);

        int poll_period = device->get_attribute_poll_period(att_name);
        TEST_LOG << "att polling period : " << poll_period << std::endl;
        assert(poll_period == 1000);

        TEST_LOG << "   subscribe 2 times to the same event (same callback) --> OK" << std::endl;

        //
        // Check that first point has been received
        //

        assert(cb.cb_executed == 2);
        assert(cb.val == 30);
        assert(cb.val_size == 4);
        TEST_LOG << "   Two first point received --> OK" << std::endl;

        //
        // Check that callback was called after a positive value change
        //

        // A trick for gdb. The thread created by omniORB for the callback execution
        // is just started during the sleep. Gdb has a breakpoint reached at each thread
        // creation to display message on the console. This breakpoint is a software
        // signal which interrupts the sleep.....
        //

        std::this_thread::sleep_for(std::chrono::seconds(1));

        device->command_inout("IOIncValue");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
        assert(cb.cb_executed == 6);
        assert(cb.val == 31);
        assert(cb.val_size == 4);

        TEST_LOG << "   Two CallBacks executed for positive absolute delta --> OK" << std::endl;

        //
        // Check that callback was called after a negative value change
        //

        device->command_inout("IODecValue");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
        assert(cb.cb_executed == 8);
        assert(cb.val == 30);
        assert(cb.val_size == 4);

        TEST_LOG << "   Two CallBacks executed for negative absolute delta --> OK" << std::endl;

        //
        // Force the attribute to throw exception
        //

        std::vector<short> data_in(2);
        data_in[0] = 1;
        data_in[1] = 1;
        di << data_in;

        device->command_inout("IOAttrThrowEx", di);

        //
        // Check that callback was called
        //

        std::this_thread::sleep_for(std::chrono::seconds(3));
        TEST_LOG << "Callback cb_err = " << cb.cb_err << std::endl;
        assert(cb.cb_err == 2);

        TEST_LOG << "   Two CallBacks executed when attribute throw exception (only once) --> OK" << std::endl;

        //
        // Attribute does not send exception any more
        //

        data_in[1] = 0;
        di << data_in;

        device->command_inout("IOAttrThrowEx", di);

        //
        // Check that the event is still received event after a try to subscribe with a null callback
        //

        bool ex = false;

        try
        {
            device->subscribe_event(att_name, Tango::CHANGE_EVENT, (CallBack *) nullptr, filters);
        }
        catch(Tango::DevFailed &e)
        {
            //            Tango::Except::print_exception(e);
            ex = true;
            std::string de(e.errors[0].desc);
        }

        device->command_inout("IOIncValue");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
        assert(cb.cb_executed == 12);
        assert(ex == true);

        TEST_LOG << "   Two CallBacks executed after a try to subscribe to one attribute with a nullptr callback --> OK"
                 << std::endl;

        //
        // unsubscribe one event
        //

        device->unsubscribe_event(eve_id1);

        TEST_LOG << "   unsubscribe one event --> OK" << std::endl;

        //
        // One more callback when value increase
        //

        std::this_thread::sleep_for(std::chrono::seconds(1));

        device->command_inout("IOIncValue");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
        assert(cb.cb_executed == 13);
        assert(cb.val == 32);
        assert(cb.val_size == 4);

        TEST_LOG << "   One CallBack executed for positive absolute delta --> OK" << std::endl;

        //
        // Check that callback was called after a negative value change
        //

        device->command_inout("IODecValue");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
        assert(cb.cb_executed == 14);
        assert(cb.val == 31);
        assert(cb.val_size == 4);

        TEST_LOG << "   One CallBack executed for negative absolute delta --> OK" << std::endl;

        //
        // unsubscribe to event
        //

        device->unsubscribe_event(eve_id2);

        TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

        //
        // With different callback
        //

        EventCallBack cb1;
        cb1.cb_executed = 0;
        cb1.cb_err = 0;
        cb1.old_sec = cb.old_usec = 0;

        EventCallBack cb2;
        cb2.cb_executed = 0;
        cb2.cb_err = 0;
        cb2.old_sec = cb.old_usec = 0;

        eve_id1 = device->subscribe_event(att_name, Tango::CHANGE_EVENT, &cb1, filters);
        eve_id2 = device->subscribe_event(att_name, Tango::CHANGE_EVENT, &cb2, filters);

        //
        // Check that first point has been received
        //

        assert(cb1.cb_executed == 1);
        assert(cb1.val == 31);
        assert(cb1.val_size == 4);
        assert(cb2.cb_executed == 1);
        assert(cb2.val == 31);
        assert(cb2.val_size == 4);

        TEST_LOG << "   subscribe 2 times to the same event (different callbacks) --> OK" << std::endl;

        //
        // One more callback when value increase
        //

        std::this_thread::sleep_for(std::chrono::seconds(1));

        device->command_inout("IOIncValue");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
        assert(cb1.cb_executed == 2);
        assert(cb1.val == 32);
        assert(cb1.val_size == 4);
        assert(cb2.cb_executed == 2);
        assert(cb2.val == 32);
        assert(cb2.val_size == 4);

        TEST_LOG << "   Two different CallBacks executed for positive absolute delta --> OK" << std::endl;

        //
        // Check that callback was called after a negative value change
        //

        device->command_inout("IODecValue");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
        assert(cb1.cb_executed == 3);
        assert(cb1.val == 31);
        assert(cb1.val_size == 4);
        assert(cb2.cb_executed == 3);
        assert(cb2.val == 31);
        assert(cb2.val_size == 4);

        TEST_LOG << "   Two different CallBacks executed for negative absolute delta --> OK" << std::endl;

        //
        // unsubscribe to events
        //

        device->unsubscribe_event(eve_id2);
        device->unsubscribe_event(eve_id1);

        TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

        //
        // Try to unsubscribe within the callback
        //

        EventUnsubCallBack cb_unsub(device);

        cb1.cb_executed = 0;
        cb2.cb_executed = 0;

        eve_id1 = device->subscribe_event(att_name, Tango::CHANGE_EVENT, &cb1, filters);
        int eve_id3 = device->subscribe_event(att_name, Tango::CHANGE_EVENT, &cb_unsub, filters);
        eve_id2 = device->subscribe_event(att_name, Tango::CHANGE_EVENT, &cb2, filters);

        cb_unsub.set_ev_id(eve_id3);

        device->command_inout("IOIncValue");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        bool unsub = false;
        try
        {
            device->unsubscribe_event(eve_id3);
        }
        catch(Tango::DevFailed &e)
        {
            //            Tango::Except::print_exception(e);
            std::string reason(e.errors[0].reason);
            if(reason == API_EventNotFound)
            {
                unsub = true;
            }
        }

        assert(unsub == true);

        device->unsubscribe_event(eve_id2);
        device->unsubscribe_event(eve_id1);

        TEST_LOG << "   Event unsubscription within the callback with two other subscribers --> OK" << std::endl;

        //
        // Stop polling
        //

        device->stop_poll_attribute(att_name);
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(-1);
    }
    catch(CORBA::Exception &ex)
    {
        Except::print_exception(ex);
        exit(-1);
    }

    delete device;

    Tango::ApiUtil::cleanup();

    return 0;
}

// NOLINTEND(*)
