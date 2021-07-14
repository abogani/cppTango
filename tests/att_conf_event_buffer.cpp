#include "common.h"

class EventCallBack : public Tango::CallBack
{
	void push_event(Tango::AttrConfEventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;

	std::string min_value;
	std::string max_value;
};

void EventCallBack::push_event(Tango::AttrConfEventData* event_data)
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
			TEST_LOG << *(event_data->attr_conf) << std::endl;
			min_value = event_data->attr_conf->min_value;
			max_value = event_data->attr_conf->max_value;
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
		std::string att_name("Double_attr_w");

//
// Check that the attribute is still not polled
//

		bool po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == false);


//
// subscribe to an attribute config event
//
		int eve_id;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		eve_id = device->subscribe_event(att_name,Tango::ATTR_CONF_EVENT,1,filters);

		{
			Tango::DevVarDoubleArray dvda(2);
			dvda.length(2);
			dvda[0] = 0.0;
			dvda[1] = 0.0;
			DeviceData d_in;
			d_in << dvda;
			device->command_inout("IOSetWAttrLimit", d_in);
		}

//
// Send 10 attribute configuration events
//
		Tango::DevVarDoubleArray dvda(2);
		dvda.length(2);
		dvda[0] = 1.0;
		dvda[1] = 10.0;

		for (int i=0; i<10; i++)
		{
			dvda[1] = dvda[1] + 1;
			DeviceData d_in;
			d_in << dvda;

			device->command_inout("IOSetWAttrLimit",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
//
// Check that only the last event was kept
//
		device->get_events(eve_id, &cb);

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 1);
		assert (cb.min_value == "0");
		assert (cb.max_value == "20");

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

		eve_id = device->subscribe_event(att_name,Tango::ATTR_CONF_EVENT,5,filters);

//
// Send 10 attribute configuration events
//
		for (int i=0; i<10; i++)
		{
			dvda[1] = dvda[1] + 1;
			DeviceData d_in;
			d_in << dvda;

			device->command_inout("IOSetWAttrLimit",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
//
// Check that only the last event was kept
//
		device->get_events(eve_id, &cb);

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 5);
		assert (cb.min_value == "0");
		assert (cb.max_value == "30");

		TEST_LOG << "   CallBack executed only for the last 5 arrived events --> OK" << std::endl;
//
// unsubscribe to the event
//
		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// Subscribe for attribute configuration events.
// Set-up the event buffers to keep all received events
//
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		eve_id = device->subscribe_event(att_name,Tango::ATTR_CONF_EVENT,ALL_EVENTS,filters);

//
// Send 10 attribute configuration events
//
		for (int i=0; i<10; i++)
		{
			dvda[1] = dvda[1] + 1;
			DeviceData d_in;
			d_in << dvda;

			device->command_inout("IOSetWAttrLimit",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
//
// Check that only the last event was kept
//
		device->get_events(eve_id, &cb);

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 11);
		assert (cb.min_value == "0");
		assert (cb.max_value == "40");

		TEST_LOG << "   CallBack executed for 11 arrived events --> OK" << std::endl;

//
// Check the attribute config event data reading as a vector
//
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

//
// Send 15 attribute configuration events
//
		for (int i=0; i<15; i++)
		{
			dvda[1] = dvda[1] + 1;
			DeviceData d_in;
			d_in << dvda;

			device->command_inout("IOSetWAttrLimit",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
//
// Check that all events were kept and can be read as a vector
//
		TEST_LOG << "event queue size = " << device->event_queue_size(eve_id) << std::endl;
		assert (device->event_queue_size(eve_id) == 15);

		AttrConfEventDataList event_list;
		device->get_events(eve_id, event_list);
		TEST_LOG << "number of events read = " << event_list.size() << std::endl;
		assert (event_list.size() == 15);

		double ref_val = 41;
		AttrConfEventDataList::iterator vpos;
		for (vpos=event_list.begin(); vpos!=event_list.end(); vpos++)
		{
			double min_value = atof ( ((*vpos)->attr_conf->min_value).c_str());
			double max_value = atof ( ((*vpos)->attr_conf->max_value).c_str());

			TEST_LOG << "min value = " << min_value << std::endl;
			TEST_LOG << "max value = " << max_value << std::endl;
			assert (max_value == ref_val);
			ref_val++;
		}
		TEST_LOG << "   Data received for 15 arrived events --> OK" << std::endl;

//
// unsubscribe to the event
//
		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// subscribe to an attribute config event several times
//

		int eve_id1,eve_id2,eve_id3;

		EventCallBack cb1;
		cb1.cb_executed = 0;
		cb1.cb_err = 0;
		cb1.old_sec = cb.old_usec = 0;

		EventCallBack cb2;
		cb2.cb_executed = 0;
		cb2.cb_err = 0;
		cb2.old_sec = cb.old_usec = 0;

		EventCallBack cb3;
		cb3.cb_executed = 0;
		cb3.cb_err = 0;
		cb3.old_sec = cb.old_usec = 0;

		eve_id1 = device->subscribe_event(att_name,Tango::ATTR_CONF_EVENT,ALL_EVENTS,filters);
		eve_id2 = device->subscribe_event(att_name,Tango::ATTR_CONF_EVENT,ALL_EVENTS,filters);
		eve_id3 = device->subscribe_event(att_name,Tango::ATTR_CONF_EVENT,ALL_EVENTS,filters);

//
// Send 15 attribute configuration events
//

		for (int i=0; i<15; i++)
		{
			dvda[1] = dvda[1] + 1;
			DeviceData d_in;
			d_in << dvda;

			device->command_inout("IOSetWAttrLimit",d_in);

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

//
// Check that all events were kept and can be read as a vector
//
		TEST_LOG << "event queue size = " << device->event_queue_size(eve_id1) << std::endl;
		assert (device->event_queue_size(eve_id1) == 16);
		assert (device->event_queue_size(eve_id2) == 16);
		assert (device->event_queue_size(eve_id3) == 16);

		AttrConfEventDataList event_list1;
		device->get_events(eve_id1, event_list1);
		TEST_LOG << "number of events read = " << event_list1.size() << std::endl;
		assert (event_list1.size() == 16);

		AttrConfEventDataList event_list2;
		device->get_events(eve_id2, event_list2);
		TEST_LOG << "number of events read = " << event_list2.size() << std::endl;
		assert (event_list2.size() == 16);

		AttrConfEventDataList event_list3;
		device->get_events(eve_id3, event_list3);
		TEST_LOG << "number of events read = " << event_list3.size() << std::endl;
		assert (event_list3.size() == 16);

		ref_val = 55;
		for (vpos=event_list1.begin(); vpos!=event_list1.end(); vpos++)
		{
			double min_value = atof ( ((*vpos)->attr_conf->min_value).c_str());
			double max_value = atof ( ((*vpos)->attr_conf->max_value).c_str());

			TEST_LOG << "min value = " << min_value << std::endl;
			TEST_LOG << "max value = " << max_value << std::endl;
			assert (max_value == ref_val);
			ref_val++;
		}

		ref_val = 55;
		for (vpos=event_list2.begin(); vpos!=event_list2.end(); vpos++)
		{
			double min_value = atof ( ((*vpos)->attr_conf->min_value).c_str());
			double max_value = atof ( ((*vpos)->attr_conf->max_value).c_str());

			TEST_LOG << "min value = " << min_value << std::endl;
			TEST_LOG << "max value = " << max_value << std::endl;
			assert (max_value == ref_val);
			ref_val++;
		}

		ref_val = 55;
		for (vpos=event_list3.begin(); vpos!=event_list3.end(); vpos++)
		{
			double min_value = atof ( ((*vpos)->attr_conf->min_value).c_str());
			double max_value = atof ( ((*vpos)->attr_conf->max_value).c_str());

			TEST_LOG << "min value = " << min_value << std::endl;
			TEST_LOG << "max value = " << max_value << std::endl;
			assert (max_value == ref_val);
			ref_val++;
		}

		TEST_LOG << "   Data received for 15 arrived events (3 subscribe) --> OK" << std::endl;

//
// unsubscribe to the events
//

		device->unsubscribe_event(eve_id1);
		device->unsubscribe_event(eve_id2);
		device->unsubscribe_event(eve_id3);

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
