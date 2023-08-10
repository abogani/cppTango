#include "common.h"

class EventCallBack : public Tango::CallBack
{
    void push_event(Tango::EventData*);

public:
    int                     cb_executed;
    int                     cb_err;
    Tango::AttrDataFormat     d_format;
};

void EventCallBack::push_event(Tango::EventData* event_data)
{
    cb_executed++;

    try
    {
        TEST_LOG << "EventCallBack::push_event(): called attribute " << event_data->attr_name << " event " << event_data->event << "\n";
        if (!event_data->err)
        {
            d_format = event_data->attr_value->get_data_format();
            TEST_LOG << "Valid data sent to callBack" << std::endl;
        }
        else
        {
            TEST_LOG << "Error send to callback" << std::endl;
            cb_err++;
        }
    }
    catch (...)
    {
        TEST_LOG << "EventCallBack::push_event(): could not extract data !\n";
    }

}

int main(int argc, char **argv)
{
    DeviceProxy *device1,*device2,*device3;

    if (argc < 4 || argc > 5)
    {
        TEST_LOG << "usage: multi_dev_event device1 device2 device3" << std::endl;
        exit(-1);
    }

    std::string device1_name = argv[1];
    std::string device2_name = argv[2];
    std::string device3_name = argv[3];

    try
    {
        device1 = new DeviceProxy(device1_name);
        device2 = new DeviceProxy(device2_name);
        device3 = new DeviceProxy(device3_name);
    }
    catch (CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    std::string att_name("Short_attr");
    std::string another_att("Event_change_tst");

    try
    {

//
// subscribe to periodic events from three devices
//

        int eve_id1,eve_id2,eve_id3,eve_id4;

        EventCallBack cb_evid1;
        cb_evid1.cb_executed = 0;
        cb_evid1.cb_err = 0;
        cb_evid1.d_format = Tango::FMT_UNKNOWN;

        EventCallBack cb_evid2;
        cb_evid2.cb_executed = 0;
        cb_evid2.cb_err = 0;
        cb_evid2.d_format = Tango::FMT_UNKNOWN;

        EventCallBack cb_evid3;
        cb_evid3.cb_executed = 0;
        cb_evid3.cb_err = 0;
        cb_evid3.d_format = Tango::FMT_UNKNOWN;

        EventCallBack cb_evid4;
        cb_evid4.cb_executed = 0;
        cb_evid4.cb_err = 0;
        cb_evid4.d_format = Tango::FMT_UNKNOWN;

        // start the polling first!
        device1->poll_attribute(att_name,500);
        device1->poll_attribute(another_att,500);
        device2->poll_attribute(att_name,500);
        device3->poll_attribute(att_name,500);

        eve_id1 = device1->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb_evid1);
        eve_id2 = device1->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb_evid2);
        eve_id3 = device2->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb_evid3);
        eve_id4 = device3->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb_evid4);

        TEST_LOG << "   subscribe to all events --> OK" << std::endl;

//
// Check that callback was called
//

// A trick for gdb. The thread created by omniORB for the callback execution
// is just started during the sleep. Gdb has a breakpoint reached at each thread
// creation to display message on the console. This breakpoint is a software
// signal which interrupts the sleep.....
//

        std::this_thread::sleep_for(std::chrono::seconds(5));

        int old_cb_evid1,old_cb_evid2,old_cb_evid3;

        TEST_LOG << "cb_evid1 executed = " << cb_evid1.cb_executed << std::endl;
        assert (cb_evid1.cb_executed > 2);
        assert (cb_evid1.cb_executed < 8);
        assert (cb_evid1.cb_err == 0);
        assert (cb_evid1.d_format == SCALAR);
        old_cb_evid1 = cb_evid1.cb_executed;

        TEST_LOG << "cb_evid2 executed = " << cb_evid2.cb_executed << std::endl;
        assert (cb_evid2.cb_executed > 2);
        assert (cb_evid2.cb_executed < 8);
        assert (cb_evid2.cb_err == 0);
        assert (cb_evid2.d_format == SCALAR);
        old_cb_evid2 = cb_evid2.cb_executed;

        TEST_LOG << "cb_evid3 executed = " << cb_evid3.cb_executed << std::endl;
        assert (cb_evid3.cb_executed > 2);
        assert (cb_evid3.cb_executed < 8);
        assert (cb_evid3.cb_err == 0);
        assert (cb_evid3.d_format == SCALAR);
        old_cb_evid3 = cb_evid3.cb_executed;

        TEST_LOG << "cb_evid4 executed = " << cb_evid4.cb_executed << std::endl;
        assert (cb_evid4.cb_executed > 2);
        assert (cb_evid4.cb_executed < 8);
        assert (cb_evid4.cb_err == 0);
        assert (cb_evid4.d_format == SCALAR);

        TEST_LOG << "   CallBack executed every 1000 mS for all events --> OK" << std::endl;

//
// unsubscribe one event
//

        device3->unsubscribe_event(eve_id4);

        std::this_thread::sleep_for(std::chrono::seconds(5));

        TEST_LOG << "cb_evid1 executed = " << cb_evid1.cb_executed << std::endl;
        assert (cb_evid1.cb_executed > old_cb_evid1 + 3);
        old_cb_evid1 = cb_evid1.cb_executed;

        TEST_LOG << "cb_evid2 executed = " << cb_evid2.cb_executed << std::endl;
        assert (cb_evid2.cb_executed > old_cb_evid2 + 3);
        old_cb_evid1 = cb_evid1.cb_executed;

        TEST_LOG << "cb_evid3 executed = " << cb_evid3.cb_executed << std::endl;
        assert (cb_evid3.cb_executed > old_cb_evid3 + 3);

        TEST_LOG << "   Event still received after first unsubscribe_event --> OK" << std::endl;

//
// unsubscribe another event
//

        device2->unsubscribe_event(eve_id3);

        std::this_thread::sleep_for(std::chrono::seconds(5));

        TEST_LOG << "cb_evid1 executed = " << cb_evid1.cb_executed << std::endl;
        assert (cb_evid1.cb_executed > old_cb_evid1 + 3);
        old_cb_evid1 = cb_evid1.cb_executed;

        TEST_LOG << "cb_evid2 executed = " << cb_evid2.cb_executed << std::endl;
        assert (cb_evid2.cb_executed > old_cb_evid2 + 3);
        old_cb_evid1 = cb_evid1.cb_executed;

        TEST_LOG << "   Event still received after second unsubscribe_event --> OK" << std::endl;

        device1->unsubscribe_event(eve_id2);

        std::this_thread::sleep_for(std::chrono::seconds(5));

        TEST_LOG << "cb_evid1 executed = " << cb_evid1.cb_executed << std::endl;
        assert (cb_evid1.cb_executed > old_cb_evid1 + 3);

        TEST_LOG << "   Event still received after third unsubscribe_event --> OK" << std::endl;

        device1->unsubscribe_event(eve_id1);

        TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// Check automatic unsubscription
//

        DeviceProxy *dev = new DeviceProxy(device2_name);

        eve_id1 = device1->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb_evid1);
        eve_id2 = dev->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb_evid2);
        eve_id3 = dev->subscribe_event("Float_attr",Tango::PERIODIC_EVENT,&cb_evid2,true);

        std::this_thread::sleep_for(std::chrono::seconds(5));

        delete dev;

        std::this_thread::sleep_for(std::chrono::seconds(15));

        TEST_LOG << "   DeviceProxy dtor automatically unsubscribe --> OK" << std::endl;

        device1->unsubscribe_event(eve_id1);
    }
    catch (Tango::DevFailed &e)
    {
        Except::print_exception(e);
    }
    catch (CORBA::Exception &ex)
    {
        Except::print_exception(ex);
    }

//
// Stop polling
//

    device1->stop_poll_attribute(att_name);
    device1->stop_poll_attribute(another_att);
    device2->stop_poll_attribute(att_name);
    device3->stop_poll_attribute(att_name);

    delete device1;
    delete device2;
    delete device3;

    return 0;
}
