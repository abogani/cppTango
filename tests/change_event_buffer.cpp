#include "common.h"

class EventCallBack : public Tango::CallBack
{
	void push_event(Tango::EventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;
	long val;
	long val_size;
};

void EventCallBack::push_event(Tango::EventData* event_data)
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
		TEST_LOG << "StateEventCallBack::push_event(): called attribute " << event_data->attr_name << " event " << event_data->event << "\n";
		if (!event_data->err)
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
			if (strcmp(event_data->errors[0].reason.in(),"bbb") == 0)
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
	DeviceProxy *device;

	if (argc == 1)
	{
		TEST_LOG << "usage: %s device" << std::endl;
		exit(-1);
	}

	std::string device_name = argv[1];

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
		std::string att_name("Event_change_tst");

//
// Test set up (stop polling and clear abs_change and rel_change attribute
// properties but restart device to take this into account)
// Set the abs_change to 1
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		DbAttribute dba(att_name,device_name);
		DbData dbd;
		DbDatum a(att_name);
		a << (short)2;
		dbd.push_back(a);
		dbd.push_back(DbDatum("abs_change"));
		dbd.push_back(DbDatum("rel_change"));
		dba.delete_property(dbd);

		dbd.clear();
		a << (short)1;
		dbd.push_back(a);
		DbDatum ch("abs_change");
		ch << (short)1;
		dbd.push_back(ch);
		dba.put_property(dbd);

		DeviceProxy adm_dev(device->adm_name().c_str());
		DeviceData di;
		di << device_name;
		adm_dev.command_inout("DevRestart",di);

		delete device;
		device = new DeviceProxy(device_name);
		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// switch on the polling first!
//
		device->poll_attribute(att_name,250);

//
// Check that the attribute is now polled at 250 mS
//

		bool po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		int poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 250);


//
// Subscribe for change events.
// Set-up the event buffers to keep only the last received event
//
		int eve_id;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;


		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,1,filters);
		TEST_LOG << "   subscribe_event --> OK" << std::endl;
//
// Send 10 change events
//
		for (int i=0; i<10; i++)
		{
			device->command_inout("IOIncValue");
			std::this_thread::sleep_for(std::chrono::milliseconds(400));
		}
//
// Check that only the last event was kept
//
		device->get_events(eve_id, &cb);

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 1);
		assert (cb.val == 40);
		TEST_LOG << "   CallBack executed only for the last arrived event --> OK" << std::endl;
//
// unsubscribe the event
//
		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;


//
// Subscribe for change events.
// Set-up the event buffers to keep only the last 5 received events
//
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,5,filters);
//
// Send 10 change events
//
		for (int i=0; i<10; i++)
		{
			device->command_inout("IOIncValue");
			std::this_thread::sleep_for(std::chrono::milliseconds(400));
		}
//
// Check that only the last 5 events were kept
//
		device->get_events(eve_id, &cb);

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 5);
		assert (cb.val == 50);
		TEST_LOG << "   CallBack executed only for the last 5 arrived events --> OK" << std::endl;
//
// unsubscribe the event
//
		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;


//
// Subscribe for change events.
// Set-up the event buffers to keep all received events
//
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,ALL_EVENTS,filters);
//
// Send 10 change events
//
		for (int i=0; i<10; i++)
		{
			device->command_inout("IOIncValue");
			std::this_thread::sleep_for(std::chrono::milliseconds(400));
		}
//
// Check that all events were kept
//
		device->get_events(eve_id, &cb);

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 11); // 1 synchrounous event at subscription +
		                               // 10 value changes
		assert (cb.val == 60);
		TEST_LOG << "   CallBack executed for 11 arrived events --> OK" << std::endl;



//
// Check the event data reading as a vector
//
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

//
// Send 15 change events
//
		for (int i=0; i<15; i++)
		{
			device->command_inout("IOIncValue");
			std::this_thread::sleep_for(std::chrono::milliseconds(400));
		}
//
// Check that all events were kept and can be read as a vector
//
		TEST_LOG << "event queue size = " << device->event_queue_size(eve_id) << std::endl;
		assert (device->event_queue_size(eve_id) == 15);

		EventDataList event_list;
		device->get_events(eve_id, event_list);
		TEST_LOG << "number of events read = " << event_list.size() << std::endl;
		assert (event_list.size() == 15);

		long ref_val = 61;
		EventDataList::iterator vpos;
		for (vpos=event_list.begin(); vpos!=event_list.end(); vpos++)
		{
			std::vector<DevLong> value;
			*((*vpos)->attr_value) >> value;

			TEST_LOG << "event value = " << value[2] << std::endl;
			assert (value[2] == ref_val);
			ref_val++;
		}
		TEST_LOG << "   Data received for 15 arrived events --> OK" << std::endl;

//
// unsubscribe the event
//
		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;


//
// Subscribe for several change events.
// Set-up the event buffers to keep only the last 5 received events
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,5,filters);
		int eve_id1 = device->subscribe_event(att_name,Tango::CHANGE_EVENT,5,filters);
		int eve_id2 = device->subscribe_event(att_name,Tango::CHANGE_EVENT,5,filters);
//
// Send 10 change events
//
		for (int i=0; i<10; i++)
		{
			device->command_inout("IOIncValue");
			std::this_thread::sleep_for(std::chrono::milliseconds(400));
		}
//
// Check that only the last 5 events were kept
//
		device->get_events(eve_id, &cb);

		assert (cb.cb_executed == 5);
		assert (cb.val == 85);

		device->get_events(eve_id1, &cb);

		assert (cb.cb_executed == 10);
		assert (cb.val == 85);

		device->get_events(eve_id2, &cb);

		assert (cb.cb_executed == 15);
		assert (cb.val == 85);

		TEST_LOG << "   Three CallBacks executed only for the last 5 arrived events --> OK" << std::endl;
//
// unsubscribe the event
//
		device->unsubscribe_event(eve_id);
		device->unsubscribe_event(eve_id1);
		device->unsubscribe_event(eve_id2);

		TEST_LOG << "   unsubscribe_events --> OK" << std::endl;
//
// Stop polling
//

		device->stop_poll_attribute(att_name);
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

	Tango::ApiUtil::cleanup();

	return 0;

}
