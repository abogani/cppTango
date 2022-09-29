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
// subscribe to a attribute config event
//

		int eve_id1,eve_id2;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		eve_id1 = device->subscribe_event(att_name,Tango::ATTR_CONF_EVENT,&cb,filters);
		eve_id2 = device->subscribe_event(att_name,Tango::ATTR_CONF_EVENT,&cb,filters);

//
// Check that the attribute is still not polled
//

		bool po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == false);

//
// The callback should have been executed once
//

		assert (cb.cb_executed == 2);

		TEST_LOG << "   subscribe_event --> OK" << std::endl;

//
// Execute the command which will fire an attribute
// config event
//

		Tango::DevVarDoubleArray dvda(2);
		dvda.length(2);
		dvda[0] = 0.0;
		dvda[1] = 0.0;
		DeviceData d_in;
		d_in << dvda;
		device->command_inout("IOSetWAttrLimit",d_in);

		dvda[0] = 1.0;
		dvda[1] = 10.0;
		d_in << dvda;
		device->command_inout("IOSetWAttrLimit",d_in);

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (cb.cb_executed == 6);
		assert (cb.min_value == "0");
		assert (cb.max_value == "10");

		TEST_LOG << "   attr_conf_event --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id1);
		device->unsubscribe_event(eve_id2);

		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;
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
