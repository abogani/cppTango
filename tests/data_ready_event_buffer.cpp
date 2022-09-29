#include "common.h"

class EventCallBack : public Tango::CallBack
{
	void push_event(Tango::DataReadyEventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;

	int ctr;
};

void EventCallBack::push_event(Tango::DataReadyEventData* event_data)
{
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
		TEST_LOG << "EventCallBack::push_event(): called attribute " << event_data->attr_name << " event " << event_data->event << "\n";
		if (!event_data->err)
		{
			TEST_LOG << "Event data: Attribute data type = " << event_data->attr_data_type << ", User ctr = " << event_data->ctr << std::endl;
			ctr = event_data->ctr;
		}
		else
		{
			TEST_LOG << "Error send to callback" << std::endl;
			Tango::Except::print_error_stack(event_data->errors);
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
		std::string att_name("Long_attr");

//
// Check that the attribute is still not polled
//

		bool po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == false);

//
// subscribe to an attribute data ready event
//

		int eve_id;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;
		cb.ctr = -1;

		eve_id = device->subscribe_event(att_name,Tango::DATA_READY_EVENT,1,filters);

//
// Send 10 attribute data ready events
//

		Tango::DevVarLongStringArray dvlsa;
		dvlsa.svalue.length(1);
		dvlsa.lvalue.length(1);
		dvlsa.lvalue[0] = 9;
		dvlsa.svalue[0] = Tango::string_dup(att_name.c_str());
		DeviceData d_in;

		for (int i=0; i<10; i++)
		{
			dvlsa.lvalue[0] = dvlsa.lvalue[0] + 1;
			d_in << dvlsa;

			device->command_inout("PushDataReady",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

//
// Check that only the last event was kept
//

		device->get_events(eve_id, &cb);

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 1);
		assert (cb.ctr == 19);

		TEST_LOG << "   CallBack executed only for the last arrived event --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// Subscribe for attribute configuration events.
// Set-up the event buffers to keep only the last 5 received events
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;
		cb.ctr = 0;

		eve_id = device->subscribe_event(att_name,Tango::DATA_READY_EVENT,5,filters);

//
// Send 10 attribute configuration events
//

		for (int i=0; i<10; i++)
		{
			dvlsa.lvalue[0] = dvlsa.lvalue[0] + 1;
			d_in << dvlsa;

			device->command_inout("PushDataReady",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

//
// Check that only the last event was kept
//

		device->get_events(eve_id, &cb);

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 5);
		assert (cb.ctr == 29);

		TEST_LOG << "   CallBack executed only for the last 5 arrived events --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// Subscribe for attribute data ready events.
// Set-up the event buffers to keep all received events
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;
		cb.ctr = 0;

		eve_id = device->subscribe_event(att_name,Tango::DATA_READY_EVENT,ALL_EVENTS,filters);

//
// Send 10 attribute data ready events
//

		for (int i=0; i<10; i++)
		{
			dvlsa.lvalue[0] = dvlsa.lvalue[0] + 1;
			d_in << dvlsa;

			device->command_inout("PushDataReady",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

//
// Check that only the last event was kept
//

		device->get_events(eve_id, &cb);

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 10);
		assert (cb.ctr == 39);

		TEST_LOG << "   CallBack executed for 10 arrived events --> OK" << std::endl;

//
// Check the data ready event data reading as a vector
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

//
// Send 15 attribute data ready events
//

		for (int i=0; i<15; i++)
		{
			dvlsa.lvalue[0] = dvlsa.lvalue[0] + 1;
			d_in << dvlsa;

			device->command_inout("PushDataReady",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

//
// Check that all events were kept and can be read as a vector
//

		TEST_LOG << "event queue size = " << device->event_queue_size(eve_id) << std::endl;
		assert (device->event_queue_size(eve_id) == 15);

		DataReadyEventDataList event_list;
		device->get_events(eve_id, event_list);
		TEST_LOG << "number of events read = " << event_list.size() << std::endl;
		assert (event_list.size() == 15);

		int ref_val = 40;
		DataReadyEventDataList::iterator vpos;
		for (vpos=event_list.begin(); vpos!=event_list.end(); vpos++)
		{
			double local_ctr = (*vpos)->ctr;

			TEST_LOG << "local_ctr = " << local_ctr << std::endl;
			assert (local_ctr == ref_val);
			ref_val++;
		}
		TEST_LOG << "   Data received for 15 arrived events --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// subscribe several times to an attribute data ready event
//

		int eve_id1,eve_id2;

		EventCallBack cb1;
		cb1.cb_executed = 0;
		cb1.cb_err = 0;
		cb1.old_sec = cb.old_usec = 0;
		cb1.ctr = -1;

		EventCallBack cb2;
		cb2.cb_executed = 0;
		cb2.cb_err = 0;
		cb2.old_sec = cb.old_usec = 0;
		cb2.ctr = -1;

		eve_id1 = device->subscribe_event(att_name,Tango::DATA_READY_EVENT,1,filters);
		eve_id2 = device->subscribe_event(att_name,Tango::DATA_READY_EVENT,1,filters);

//
// Send 10 attribute data ready events
//

		Tango::DevVarLongStringArray dvlsa1;
		dvlsa1.svalue.length(1);
		dvlsa1.lvalue.length(1);
		dvlsa1.lvalue[0] = 9;
		dvlsa1.svalue[0] = Tango::string_dup(att_name.c_str());
		DeviceData d_in1;

		for (int i=0; i<10; i++)
		{
			dvlsa1.lvalue[0] = dvlsa1.lvalue[0] + 1;
			d_in1 << dvlsa1;

			device->command_inout("PushDataReady",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

//
// Check that only the last event was kept
//

		device->get_events(eve_id1, &cb1);
		device->get_events(eve_id2, &cb2);

		TEST_LOG << "cb excuted = " << cb1.cb_executed << std::endl;
		assert (cb1.cb_executed == 1);
		assert (cb1.ctr == 54);
		assert (cb2.cb_executed == 1);
		assert (cb2.ctr == 54);

		TEST_LOG << "   Several CallBack executed only for the last arrived event --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id1);
		device->unsubscribe_event(eve_id2);
		TEST_LOG << "   unsubscribe_events --> OK" << std::endl;

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
