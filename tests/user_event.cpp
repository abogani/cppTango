#include "common.h"

class EventCallBack : public Tango::CallBack
{
	void push_event(Tango::EventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;
};

class EventEncodedCallBack : public Tango::CallBack
{
	void push_event(Tango::EventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;
};

void EventCallBack::push_event(Tango::EventData* event_data)
{
	std::vector<long> value;
//	Tango::DevState sta;
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

			*(event_data->attr_value) >> value;
//			*(event_data->attr_value) >> sta;
			TEST_LOG << "CallBack vector value size : " << value.size() << std::endl;
//			TEST_LOG << "CallBack vector value size : " << sta << std::endl;
		}
		else
		{
			TEST_LOG << "Error send to callback" << std::endl;
//			Tango::Except::print_error_stack(event_data->errors);
			if (strcmp(event_data->errors[0].reason.in(),"aaa") == 0)
				cb_err++;
		}
	}
	catch (...)
	{
		TEST_LOG << "EventCallBack::push_event(): could not extract data !\n";
	}

}

void EventEncodedCallBack::push_event(Tango::EventData* event_data)
{
	struct timeval now_timeval = Tango::make_timeval(std::chrono::system_clock::now());

	TEST_LOG << "date : tv_sec = " << now_timeval.tv_sec;
	TEST_LOG << ", tv_usec = " << now_timeval.tv_usec << std::endl;

	cb_executed++;

	try
	{
		TEST_LOG << "EventEncodedCallBack::push_event(): called attribute " << event_data->attr_name << " event " << event_data->event << "\n";
		if (!event_data->err)
		{
			TEST_LOG << "Valid data received" << std::endl;
			TEST_LOG << *(event_data->attr_value) << std::endl;
		}
		else
		{
			TEST_LOG << "Error send to callback" << std::endl;
			if (strcmp(event_data->errors[0].reason.in(),"aaa") == 0)
				cb_err++;
		}
	}
	catch (...)
	{
		TEST_LOG << "EventEncodedCallBack::push_event(): could not extract data !\n";
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

		std::string att_name("event_change_tst");

//
// Test set up (stop polling of this attribute but start to poll another
// attribute)
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		DeviceAttribute dap = device->read_attribute("String_attr");

//
// subscribe to a user event
//

		int eve_id;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		eve_id = device->subscribe_event(att_name,Tango::USER_EVENT,&cb,filters);

//
// Check that the attribute is still not polled
//

		bool po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == false);

//
// The callback should have been executed once
//

		assert (cb.cb_executed == 1);

		TEST_LOG << "   subscribe_event --> OK" << std::endl;

//
// Fire the event
//

		device->command_inout("IOPushEvent");
		device->command_inout("IOPushEvent");
		device->command_inout("IOPushEvent");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (cb.cb_executed == 4);
		TEST_LOG << "   user_event --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

#ifndef COMPAT
//
// subscribe to a user event for an attribute of the DevEncoded data type
//

		att_name = "encoded_attr";
		EventEncodedCallBack enc_cb;
		enc_cb.cb_executed = 0;
		enc_cb.cb_err = 0;
		enc_cb.old_sec = cb.old_usec = 0;

		eve_id = device->subscribe_event(att_name,Tango::USER_EVENT,&enc_cb,filters);

//
// The callback should have been executed once
//

		assert (enc_cb.cb_executed == 1);

		TEST_LOG << "   subscribe_event (DevEncoded data type) --> OK" << std::endl;

//
// Fire the event
//

		device->command_inout("IOPushDevEncodedEvent");
		device->command_inout("IOPushDevEncodedEvent");
		device->command_inout("IOPushDevEncodedEvent");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (enc_cb.cb_executed == 4);
		TEST_LOG << "   user_event (DevEncoded data type) --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event (DevEncoded data type) --> OK" << std::endl;
#endif
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
