#include "common.h"
#include <cstdint>

class EventCallBack : public Tango::CallBack
{
	void push_event(Tango::EventData*);

public:
	int 					cb_executed;
	int 					cb_err;
	int 					cb_err_out_of_sync;
	int 					a_cb_err;
	int 					old_sec,old_usec;
	std::int_least64_t 		delta_msec;
	Tango::AttrDataFormat 	d_format;
};

void EventCallBack::push_event(Tango::EventData* event_data)
{
	short value;
	struct timeval now_timeval = Tango::make_timeval(std::chrono::system_clock::now());

	TEST_LOG << "date : tv_sec = " << now_timeval.tv_sec;
	TEST_LOG << ", tv_usec = " << now_timeval.tv_usec << std::endl;

	delta_msec = ((now_timeval.tv_sec - old_sec) * 1000) + ((now_timeval.tv_usec - old_usec) / 1000);

	old_sec = now_timeval.tv_sec;
	old_usec = now_timeval.tv_usec;

	TEST_LOG << "delta_msec = " << delta_msec << std::endl;

	cb_executed++;

	try
	{
		TEST_LOG << "StateEventCallBack::push_event(): called attribute " << event_data->attr_name << " event " << event_data->event << "\n";
		if (!event_data->err)
		{

			*(event_data->attr_value) >> value;
			d_format = event_data->attr_value->get_data_format();
			TEST_LOG << "CallBack value " << value << std::endl;
		}
		else
		{
			TEST_LOG << "Error send to callback" << std::endl;
			a_cb_err++;
//			Tango::Except::print_error_stack(event_data->errors);
			if (strcmp(event_data->errors[0].reason.in(),"aaa") == 0)
				cb_err++;
			else if (strcmp(event_data->errors[0].reason.in(),API_PollThreadOutOfSync) == 0)
				cb_err_out_of_sync++;
		}
	}
	catch (...)
	{
		TEST_LOG << "EventCallBack::push_event(): could not extract data !\n";
	}

}


class EventCallBackSub : public Tango::CallBack
{
	void push_event(Tango::EventData*);

public:
    std::string                  dev_name;
    std::string                  attr_name;
	int 					cb_executed;
    int                     cb_executed_sub;
	int 					cb_err;
	DeviceProxy             *dev2;
	int 					ev_id2;
};

void EventCallBackSub::push_event(Tango::EventData* event_data)
{
	cb_executed++;
	TEST_LOG << "I have received an event for attribute " << event_data->attr_name << std::endl;

	if (event_data->err == true)
	{
		TEST_LOG << "The event is an error" << std::endl;
		Tango::Except::print_error_stack(event_data->errors);
        cb_err++;
	}
	else
	{
        if (cb_executed == 2)
        {
            TEST_LOG << "Going to subscribe to another event" << std::endl;
            try
            {
                dev2 = new Tango::DeviceProxy(dev_name);
                TEST_LOG << "DeviceProxy object created" << std::endl;
                ev_id2 = dev2->subscribe_event(attr_name,Tango::PERIODIC_EVENT,this,true);
                TEST_LOG << "Subscribed to event, id = " << ev_id2 << std::endl;
            }
            catch (Tango::DevFailed &e)
            {
                Tango::Except::print_exception(e);
            }
        }

        if (cb_executed == 12)
        {
            try
            {
                TEST_LOG << "Going to unsubscribe, ev_id2 = " << ev_id2 << std::endl;
                dev2->unsubscribe_event(ev_id2);
                delete dev2;
            }
            catch (Tango::DevFailed &e)
            {
                Tango::Except::print_exception(e);
            }
        }

        std::string::size_type pos = event_data->attr_name.find(dev_name);
        if (pos != std::string::npos)
            cb_executed_sub++;
	}
}


int main(int argc, char **argv)
{
	DeviceProxy *device;

	if (argc < 3 || argc > 4)
	{
		TEST_LOG << "usage: %s device1 device2" << std::endl;
		exit(-1);
	}

	std::string device_name = argv[1];
  std::string device_name_sub = argv[2];

	try
	{
		device = new DeviceProxy(device_name);
	}
	catch (CORBA::Exception &e)
	{
		Except::print_exception(e);
		exit(1);
	}

	TEST_LOG << std::endl << "new DeviceProxy(" << device->name() << ") returned" << std::endl << std::endl;

	try
	{

		std::string att_name("Short_attr");
		std::string another_att("Event_change_tst");

//
// Test set up (stop polling and clear event_period attribute property but
// restart device to take this into account)
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		if (device->is_attribute_polled(another_att))
			device->stop_poll_attribute(another_att);
		DbAttribute dba(att_name,device_name);
		DbData dbd;
		DbDatum a(att_name);
		a << (short)1;
		dbd.push_back(a);
		dbd.push_back(DbDatum("event_period"));

		dba.delete_property(dbd);
		DeviceProxy adm_dev(device->adm_name().c_str());
		DeviceData di;
		di << device_name;
		adm_dev.command_inout("DevRestart",di);

		delete device;
		device = new DeviceProxy(device_name);
		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// subscribe to a periodic event
//

		int eve_id;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.a_cb_err = 0;
		cb.cb_err_out_of_sync = 0;
		cb.old_sec = cb.old_usec = 0;
		cb.d_format = Tango::FMT_UNKNOWN;

		// start the polling first!
		device->poll_attribute(att_name,500);

		eve_id = device->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb,filters);

//
// Check that the attribute is now polled at 500 mS
//

		bool po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		int poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 500);

		TEST_LOG << "   subscribe_event --> OK" << std::endl;

//
// Check that callback was called
//

// A trick for gdb. The thread created by omniORB for the callback execution
// is just started during the sleep. Gdb has a breakpoint reached at each thread
// creation to display message on the console. This breakpoint is a software
// signal which interrupts the sleep.....
//

		std::this_thread::sleep_for(std::chrono::seconds(5));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed > 2);
		assert (cb.cb_executed < 7);
		assert (cb.delta_msec > 900);
		assert (cb.delta_msec < 1100);
		assert (cb.a_cb_err == 0);
		assert (cb.d_format == SCALAR);

		TEST_LOG << "   CallBack executed every 1000 mS --> OK" << std::endl;

//
// Force the attribute to throw exception
//

		std::vector<short> data_in(2);
		data_in[0] = 0;
		data_in[1] = 1;
		di << data_in;

		device->command_inout("IOAttrThrowEx",di);

//
// Check that callback was called
//

		std::this_thread::sleep_for(std::chrono::seconds(3));
		TEST_LOG << "Callback cb_err = " << cb.cb_err << std::endl;
		assert (cb.cb_err > 2);
		assert (cb.cb_err < 5);
		assert (cb.delta_msec > 950);
		assert (cb.delta_msec < 1050);

		TEST_LOG << "   CallBack executed when attribute throw exception every 1000 mS --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// Attribute does not send exception any more
//

		data_in[1] = 0;
		di << data_in;

		device->command_inout("IOAttrThrowEx",di);

//
// subscribe to a periodic event with a filter
//

#ifdef NOTIFD
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;
		filters.push_back("$counter % 2 == 0");

		eve_id = device->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb,filters);
		TEST_LOG << "   subscribe_event (with modulo filter) --> OK" << std::endl;

//
// Check that callback was called
//

		std::this_thread::sleep_for(std::chrono::seconds(6));
		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed > 2);
		assert (cb.cb_executed < 5);
		assert (cb.delta_msec > 1900);
		assert (cb.delta_msec < 2100);

		TEST_LOG << "   CallBack executed every 2000 mS --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event (with filter) --> OK" << std::endl;
#endif

//
// Change polling period to 200mS
//

		device->poll_attribute(att_name,200);

//
// subscribe to a periodic event
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.d_format = FMT_UNKNOWN;
		filters.clear();

		eve_id = device->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb,filters);

//
// Check that the attribute is still polled at 200 mS
//

		po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 200);

		TEST_LOG << "   subscribe_event (with attribute already polled) --> OK" << std::endl;

//
// Check that callback was called
//

		std::this_thread::sleep_for(std::chrono::seconds(3));
		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed > 2);
		assert (cb.cb_executed < 5);
		assert (cb.delta_msec > 900);
		assert (cb.delta_msec < 1100);
		assert (cb.d_format == SCALAR);

		TEST_LOG << "   CallBack executed every 1000 mS --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// Stop polling
//

		device->stop_poll_attribute(att_name);

//
// Change the periodic event "event_period" attribute property to
// 600 mS
//

		DbAttribute db_att(att_name,device_name);
		DbData db_dat;
		DbDatum dat(att_name);
		dat << (DevLong)1;
		db_dat.push_back(dat);
		DbDatum dat1("event_period");
		dat1 << (DevLong)600;
		db_dat.push_back(dat1);

		db_att.put_property(db_dat);

//
// Restart device
//

		DeviceProxy adm_dev_2(device->adm_name().c_str());
		di << device_name;
		adm_dev_2.command_inout("DevRestart",di);

		delete device;
		device = new DeviceProxy(device_name);

		std::this_thread::sleep_for(std::chrono::seconds(2));

//
// Poll device at 200 mS
//

		device->poll_attribute(att_name,200);

		std::this_thread::sleep_for(std::chrono::milliseconds(20));

//
// subscribe to a periodic event
//

		cb.cb_executed = 0;
		cb.cb_err = 0;

		eve_id = device->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb,filters);

//
// Check that the attribute is  polled at 200 mS
//

		po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 200);

		TEST_LOG << "   subscribe_event (with event_period property set) --> OK" << std::endl;

//
// Check that callback was called
//

//		std::this_thread::sleep_for(std::chrono::seconds(3));
		std::this_thread::sleep_for(std::chrono::seconds(6));
		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed >= 5);
		assert (cb.cb_executed < 12);
		assert (cb.delta_msec > 500);
		assert (cb.delta_msec < 700);

		TEST_LOG << "   CallBack executed every 600 mS --> OK" << std::endl;

//
// Change polling period to 600mS
//

		device->poll_attribute(att_name,600);

//
// Poll another attribute
//

		device->poll_attribute(another_att,600);

		std::this_thread::sleep_for(std::chrono::seconds(2));

//
// Force event_change_tst to have a long reading time.
// This will force out of sync exception on Short_attr attribute
//

/*		std::vector<short> data_in_out(2);
		data_in_out[0] = 3;
		data_in_out[1] = 1;
		di << data_in_out;

		device->command_inout("IOAttrThrowEx",di);

		std::this_thread::sleep_for(std::chrono::seconds(5));

		assert (cb.cb_err_out_of_sync > 2);

		TEST_LOG << "   Receiving out_of_sync exception --> OK" << std::endl;

//
// No out of sync exception any more
//

		data_in_out[1] = 0;
		di << data_in_out;

		device->command_inout("IOAttrThrowEx",di);

		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// Clear error flags and wait for some callbacks
//

		cb.a_cb_err = cb.cb_err = cb.cb_err_out_of_sync = 0;
		cb.cb_executed = 0;

		std::this_thread::sleep_for(std::chrono::seconds(3));

		assert (cb.cb_executed > 2);
		assert (cb.a_cb_err == 0);
		assert (cb.cb_err == 0);
		assert (cb.cb_err_out_of_sync == 0);

		TEST_LOG << "   Back to periodic event after out_of_sync exception --> OK" << std::endl;*/

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// subscribe again to test subsription in CB an dpoll attribute on second device
//

        EventCallBackSub cb_sub;
        cb_sub.cb_err = 0;
        cb_sub.cb_executed = 0;
        cb_sub.cb_executed_sub = 0;
        cb_sub.dev_name = device_name_sub;
        cb_sub.attr_name = att_name;

        DeviceProxy dev_sub(device_name_sub);
        dev_sub.poll_attribute(att_name,500);

		eve_id = device->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb_sub);

        std::this_thread::sleep_for(std::chrono::seconds(7));

        device->unsubscribe_event(eve_id);
        dev_sub.stop_poll_attribute(att_name);

        TEST_LOG << "cb_executed = " << cb_sub.cb_executed << std::endl;
        TEST_LOG << "cb_executed_sub = " << cb_sub.cb_executed_sub << std::endl;
        TEST_LOG << "cb_err = " << cb_sub.cb_err << std::endl;

		assert (cb_sub.cb_executed > 10);
        assert (cb_sub.cb_executed_sub >= 3);
		assert (cb_sub.cb_err == 0);

        TEST_LOG << "   subscription / unsubscription in event callback --> OK" << std::endl;

//
// Stop polling
//

		device->stop_poll_attribute(att_name);
		device->stop_poll_attribute(another_att);

	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}
	catch (CORBA::Exception &ex)
	{
		Except::print_exception(ex);
		exit(-1);
	}


	delete device;

	return 0;
}
