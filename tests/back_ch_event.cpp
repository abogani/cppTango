#include "common.h"

class EventCallBack : public Tango::CallBack
{
	void push_event(Tango::EventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;
};

void EventCallBack::push_event(Tango::EventData* event_data)
{
	long value;
	struct timeval now_timeval = Tango::make_timeval(std::chrono::system_clock::now());

	TEST_LOG << "Callback date : tv_sec = " << now_timeval.tv_sec;
	TEST_LOG << ", tv_usec = " << now_timeval.tv_usec << std::endl;

	auto delta_msec = ((now_timeval.tv_sec - old_sec) * 1000) + ((now_timeval.tv_usec - old_usec) / 1000);

	old_sec = now_timeval.tv_sec;
	old_usec = now_timeval.tv_usec;

	TEST_LOG << "delta_msec = " << delta_msec << std::endl;

	cb_executed++;
	try
	{
//		TEST_LOG << "StateEventCallBack::push_event(): called attribute " << event_data->attr_name << " event " << event_data->event << "\n";
		if (!event_data->err)
		{

			*(event_data->attr_value) >> value;
			TEST_LOG << "CallBack value " << value << std::endl;
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

int main(int argc, char **argv)
{
	DeviceProxy *device;

	if ((argc == 1) || (argc == 2))
	{
		TEST_LOG << "usage: %s device sleeping_time" << std::endl;
		exit(-1);
	}

	std::string device_name = argv[1];
	long sleeping_time = atol(argv[2]);

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

		std::string att_name("long_attr");

//
// Test set up (stop polling and clear event_period attribute property but
// restart device to take this into account)
//

//		if (device->is_attribute_polled(att_name))
//			device->stop_poll_attribute(att_name);

//
// subscribe to a periodic event
//

		int eve_id;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		filters.push_back("$delta_change_abs >= 2 or $delta_change_abs <= -2");
		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,&cb,filters);

		sleep(sleeping_time);

		device->unsubscribe_event(eve_id);
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
