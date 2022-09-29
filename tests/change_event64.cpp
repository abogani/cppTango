#include "common.h"

class EventCallBack : public Tango::CallBack
{
	void push_event(Tango::EventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;
	DevLong64 val;
	long val_size;
};

void EventCallBack::push_event(Tango::EventData* event_data)
{
	std::vector<DevLong64> value;
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
			val = value[0];
			val_size = value.size();
			TEST_LOG << "Callback value " << val << std::endl;
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
		std::string att_name("Event64_change_tst");

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
// subscribe to a change event
//

		int eve_id;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		// switch on the polling first!
		device->poll_attribute(att_name,1000);
		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,&cb,filters);

//
// Check that the attribute is now polled at 1000 mS
//

		bool po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		int poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 1000);

		TEST_LOG << "   subscribe_event --> OK" << std::endl;

//
// Check that first point has been received
//

		assert (cb.cb_executed == 1);
		assert (cb.val == 0x800000000LL);
		assert (cb.val_size == 2);
		TEST_LOG << "   first point received --> OK" << std::endl;

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
		assert (cb.cb_executed == 3);
		assert (cb.val == 0x800000001LL);
		assert (cb.val_size == 2);

		TEST_LOG << "   CallBack executed for positive absolute delta --> OK" << std::endl;

//
// Check that callback was called after a negative value change
//

		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 4);
		assert (cb.val == 0x800000000LL);
		assert (cb.val_size == 2);

		TEST_LOG << "   CallBack executed for negative absolute delta --> OK" << std::endl;

//
// Check that callback was called after adding one elt to the spectrum
//

		device->command_inout("IOAddOneElt");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 5);
		assert (cb.val == 0x800000000LL);
		assert (cb.val_size == 3);

		TEST_LOG << "   CallBack executed when spectrum size increases --> OK" << std::endl;

//
// Check that callback was called after removing one elt to the spectrum
//

		device->command_inout("IORemoveOneElt");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 6);
		assert (cb.val == 0x800000000LL);
		assert (cb.val_size == 2);

		TEST_LOG << "   CallBack executed when spectrum size decreases --> OK" << std::endl;

//
// Stop polling
//

		device->stop_poll_attribute(att_name);

//
// We should have receive one error event
//

		std::this_thread::sleep_for(std::chrono::seconds(1));

		TEST_LOG << "cb err = " << cb.cb_err << std::endl;
		assert (cb.cb_err == 1);

		TEST_LOG << "   Error received in callback if we stop polling --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

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

	Tango::ApiUtil::cleanup();

	return 0;
}
