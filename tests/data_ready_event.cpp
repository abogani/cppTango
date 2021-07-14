#include "common.h"

class EventCallBack : public Tango::CallBack
{
	void push_event(Tango::DataReadyEventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;

	int user_ctr;

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
			user_ctr = event_data->ctr;
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
//
// Subscribe to an attribute which is not data ready event enabled should fail
//

		std::string att_name("Short_attr");

		int eve_id1,eve_id2;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;
		cb.user_ctr = 0;

		bool except = true;

		try
		{
			eve_id1 = device->subscribe_event(att_name,Tango::DATA_READY_EVENT,&cb,filters);
			except = false;
		}
		catch (Tango::DevFailed&)
		{
//			Tango::Except::print_exception(e);
		}

		assert (except == true);

		att_name = "Long_attr";


//
// subscribe to a data ready event
//

		eve_id1 = device->subscribe_event(att_name,Tango::DATA_READY_EVENT,&cb,filters);
		eve_id2 = device->subscribe_event(att_name,Tango::DATA_READY_EVENT,&cb,filters);

//
// Check that the attribute is still not polled
//

		bool po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == false);

//
// The callback should not have been executed once
//

		assert (cb.cb_executed == 0);

		TEST_LOG << "   subscribe_event --> OK" << std::endl;

//
// Execute the command which will fire an attribute
// data ready event
//

		Tango::DevVarLongStringArray dvlsa;
		dvlsa.svalue.length(1);
		dvlsa.lvalue.length(1);
		dvlsa.lvalue[0] = 10;
		dvlsa.svalue[0] = Tango::string_dup(att_name.c_str());
		DeviceData d_in;
		d_in << dvlsa;
		device->command_inout("PushDataReady",d_in);

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (cb.cb_executed == 2);
		assert (cb.user_ctr == 10);

//
// Redo the command several times
//

		dvlsa.lvalue[0] = 15;
		d_in << dvlsa;
		device->command_inout("PushDataReady",d_in);

		dvlsa.lvalue[0] = 16;
		d_in << dvlsa;
		device->command_inout("PushDataReady",d_in);

		dvlsa.lvalue[0] = 17;
		d_in << dvlsa;
		device->command_inout("PushDataReady",d_in);

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (cb.cb_executed == 8);
		assert (cb.user_ctr == 17);

		TEST_LOG << "   data_ready_event --> OK" << std::endl;

//
// Server push event on a non-exiting attribute
//

		bool received_err = false;
		std::string err_reason;

		dvlsa.svalue[0] = Tango::string_dup("bidon");
		d_in << dvlsa;

		try
		{
			device->command_inout("PushDataReady",d_in);
		}
		catch (Tango::DevFailed &e)
		{
			received_err = true;
			err_reason = e.errors[0].reason;
		}

		assert (received_err == true);
		assert (err_reason == API_AttrNotFound);

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
