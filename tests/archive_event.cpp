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
//			Tango::Except::print_error_stack(event_data->errors);
			if (strcmp(event_data->errors[0].reason.in(),"bbb") == 0)
				cb_err++;
		}
	}
	catch (Tango::DevFailed &e)
	{
		Tango::Except::print_exception(e);
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
		a << (short)3;
		dbd.push_back(a);
		dbd.push_back(DbDatum("archive_abs_change"));
		dbd.push_back(DbDatum("archive_rel_change"));
		dbd.push_back(DbDatum("archive_period"));
		dba.delete_property(dbd);

		dbd.clear();
		a << (short)2;
		dbd.push_back(a);
		DbDatum ch("archive_abs_change");
		ch << (short)1;
		dbd.push_back(ch);
		DbDatum p("archive_period");
		p << (short)10000;
		dbd.push_back(p);
		dba.put_property(dbd);

		DeviceProxy adm_dev(device->adm_name().c_str());
		DeviceData di;
		di << device_name;
		adm_dev.command_inout("DevRestart",di);

		delete device;
		device = new DeviceProxy(device_name);
		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// subscribe to a archive event
//

		int eve_id;
		std::vector<std::string> filters;
		EventCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		bool po;
		int poll_period;

		// start the polling first!
		device->poll_attribute(att_name,1000);

		eve_id = device->subscribe_event(att_name,Tango::ARCHIVE_EVENT,&cb,filters);

//
// Check that the attribute is now polled at 1000 mS
//

		po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 1000);

		TEST_LOG << "   subscribe_event --> OK" << std::endl;

//
// Check that first point has been received
//

		assert (cb.cb_executed == 1);
		assert (cb.val == 30);
		assert (cb.val_size == 4);
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
		assert (cb.val == 31);
		assert (cb.val_size == 4);

		TEST_LOG << "   CallBack executed for positive absolute delta --> OK" << std::endl;

//
// Check that callback was called after a negative value change
//

		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 4);
		assert (cb.val == 30);
		assert (cb.val_size == 4);

		TEST_LOG << "   CallBack executed for negative absolute delta --> OK" << std::endl;

//
// Check that callback was called after adding one elt to the spectrum
//

		device->command_inout("IOAddOneElt");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 5);
		assert (cb.val == 30);
		assert (cb.val_size == 5);

		TEST_LOG << "   CallBack executed when spectrum size increases --> OK" << std::endl;

//
// Check that callback was called after removing one elt to the spectrum
//

		device->command_inout("IORemoveOneElt");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 6);
		assert (cb.val == 30);
		assert (cb.val_size == 4);

		TEST_LOG << "   CallBack executed when spectrum size decreases --> OK" << std::endl;

//
// Force the attribute to throw exception
//

		std::vector<short> data_in(2);
		data_in[0] = 1;
		data_in[1] = 1;
		di << data_in;

		device->command_inout("IOAttrThrowEx",di);

//
// Check that callback was called with error
// two times: one for error detection and the other one due to the
// periodic call
//

		std::this_thread::sleep_for(std::chrono::seconds(3));
		TEST_LOG << "Callback cb_err = " << cb.cb_err << std::endl;
		assert ( (cb.cb_err == 1) || (cb.cb_err == 2));

		TEST_LOG << "   CallBack executed when attribute throw exception only once --> OK" << std::endl;

//
// Attribute does not send exception any more
//

		data_in[1] = 0;
		di << data_in;

		device->command_inout("IOAttrThrowEx",di);

//
// Wait for the periodic part of the event (default = 10 sec)
// Wait one second for the callback sent because exception not thrown any more
//

		std::this_thread::sleep_for(std::chrono::seconds(1));
		long nb_cb = cb.cb_executed;
		std::this_thread::sleep_for(std::chrono::seconds(12));

		assert (cb.cb_executed == (nb_cb + 1));
		TEST_LOG << "   CallBack executed for the periodic part of the event --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;
//
// Change the event parameters. This means restart the device to take
// this into account
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		dbd.clear();
		a << (short)3;
		dbd.push_back(a);
		dbd.push_back(DbDatum("archive_abs_change"));
		dbd.push_back(DbDatum("archive_rel_change"));
		dbd.push_back(DbDatum("archive_period"));
		dba.delete_property(dbd);
exit(0);
		dbd.clear();
		a << (short)2;
		dbd.push_back(a);
		DbDatum drel("archive_rel_change");
		drel << (short)10;
		dbd.push_back(drel);
		DbDatum drel1("archive_period");
		drel1 << (short)5000;
		dbd.push_back(drel1);
		dba.put_property(dbd);

		DeviceProxy adm_dev_1 = device->adm_name().c_str();
		di << device_name;
		adm_dev_1.command_inout("DevRestart",di);

		delete device;
		device = new DeviceProxy(device_name);
		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// Poll attribute at 500 mS
//

		device->poll_attribute(att_name,500);

//
// subscribe to an archive event
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		eve_id = device->subscribe_event(att_name,Tango::ARCHIVE_EVENT,&cb,filters);

//
// Check that the attribute is still polled at 500 mS
//

		po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 500);

		TEST_LOG << "   subscribe_event (with relative change) --> OK" << std::endl;

//
// Check that first point has been received
//

		assert (cb.cb_executed == 1);
		assert (cb.val == 30);
		assert (cb.val_size == 4);
		TEST_LOG << "   first point received (with relative change) --> OK" << std::endl;

//
// Generates a positive change
//

		std::this_thread::sleep_for(std::chrono::seconds(1));

		device->command_inout("IOIncValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 1);

		device->command_inout("IOIncValue");
		device->command_inout("IOIncValue");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (cb.cb_executed == 2);
		assert (cb.val == 33);
		assert (cb.val_size == 4);

		TEST_LOG << "   CallBack executed for positive relative delta --> OK" << std::endl;

//
// Generates a negative change (10 % of 33 is 3.3)
// Note that like archive_period is set to 5 seconds, we should have received
// an archive periodic callback
//

		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(3));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;

// 3 instead of 2 due to the archive_period property

		assert (cb.cb_executed == 3);

		device->command_inout("IODecValue");
		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (cb.cb_executed == 3);

		device->command_inout("IODecValue");
		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		assert (cb.cb_executed == 4);
		assert (cb.val == 28);
		assert (cb.val_size == 4);

		TEST_LOG << "   CallBack executed for negative relative delta --> OK" << std::endl;

//
// Force the attribute to throw exception
//

		data_in[0] = 1;
		data_in[1] = 1;
		di << data_in;

		device->command_inout("IOAttrThrowEx",di);

//
// Check that callback was called
//

		std::this_thread::sleep_for(std::chrono::seconds(2));
		TEST_LOG << "Callback cb_err = " << cb.cb_err << std::endl;
		assert (cb.cb_err == 1);

		TEST_LOG << "   CallBack executed when attribute throw exception only once --> OK" << std::endl;

//
// Attribute does not send exception any more
//

		data_in[1] = 0;
		di << data_in;

		device->command_inout("IOAttrThrowEx",di);

//
// Wait for the periodic part of the event (is now set to 5 sec)
//

		std::this_thread::sleep_for(std::chrono::seconds(1));
		nb_cb = cb.cb_executed;
		std::this_thread::sleep_for(std::chrono::seconds(7));

		assert (cb.cb_executed >= (nb_cb + 1));
		TEST_LOG << "   CallBack executed for the periodic part of the event (archive_period set) --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// Change the event parameters. This means restart the device to take
// this into account
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		dbd.clear();
		a << (short)3;
		dbd.push_back(a);
		dbd.push_back(DbDatum("archive_abs_change"));
		dbd.push_back(DbDatum("archive_rel_change"));
		dbd.push_back(DbDatum("archive_period"));
		dba.delete_property(dbd);

		dbd.clear();
		a << (short)1;
		dbd.push_back(a);
		dbd.push_back(ch);
		dba.put_property(dbd);

		DeviceProxy adm_dev_2 = device->adm_name().c_str();
		di << device_name;
		adm_dev_2.command_inout("DevRestart",di);

		delete device;
		device = new DeviceProxy(device_name);
		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// subscribe to a archive event with a filter
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;
		filters.push_back("$delta_change_abs >= 2 or $delta_change_abs <= -2");

		// start the polling first!
		device->poll_attribute(att_name,1000);
#ifdef NOTIFD
		eve_id = device->subscribe_event(att_name,Tango::ARCHIVE_EVENT,&cb,filters);

//
// Check that the attribute is still polled at 1000 mS
//

		po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 1000);

		TEST_LOG << "   subscribe_event (with filter) --> OK" << std::endl;

//
// Check that first point has been received
//

		assert (cb.cb_executed == 1);
		assert (cb.val == 30);
		assert (cb.val_size == 4);
		TEST_LOG << "   first point received (with filter) --> OK" << std::endl;

//
// Generates a positive change
//

		std::this_thread::sleep_for(std::chrono::seconds(1));

		device->command_inout("IOIncValue");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 1);

// Let's hope the polling thread will not start just between the two command_inout

		device->command_inout("IOIncValue");
		device->command_inout("IOIncValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		assert (cb.cb_executed == 2);
		assert (cb.val == 33);
		assert (cb.val_size == 4);

		TEST_LOG << "   CallBack executed for positive delta (with filter) --> OK" << std::endl;

//
// Generates a negative change
//

		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 2);

		device->command_inout("IODecValue");
		device->command_inout("IODecValue");
		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		assert (cb.cb_executed == 3);
		assert (cb.val == 29);
		assert (cb.val_size == 4);

		TEST_LOG << "   CallBack executed for negative delta (with filter) --> OK" << std::endl;

//
// Check that callback was called after adding one elt to the spectrum
//

		device->command_inout("IOAddOneElt");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 4);
		assert (cb.val == 29);
		assert (cb.val_size == 5);

		TEST_LOG << "   CallBack executed when spectrum size increases (with filter) --> OK" << std::endl;

//
// Check that callback was called after removing one elt to the spectrum
//

		device->command_inout("IORemoveOneElt");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 5);
		assert (cb.val == 29);
		assert (cb.val_size == 4);

		TEST_LOG << "   CallBack executed when spectrum size decreases (with filter) --> OK" << std::endl;

//
// Force the attribute to throw exception
//

		data_in[0] = 1;
		data_in[1] = 1;
		di << data_in;

		device->command_inout("IOAttrThrowEx",di);

//
// Check that callback was called
//

		std::this_thread::sleep_for(std::chrono::seconds(3));
		TEST_LOG << "Callback cb_err = " << cb.cb_err << std::endl;
		assert (cb.cb_err == 1);

		TEST_LOG << "   CallBack executed when attribute throw exception only once (with filter) --> OK" << std::endl;

//
// Attribute does not send exception any more
//

		data_in[0] = 1;
		data_in[1] = 0;
		di << data_in;

		device->command_inout("IOAttrThrowEx",di);

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;
#endif

//
// Change the event parameters. This means restart the device to take
// this into account
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		dbd.clear();
		a << (short)3;
		dbd.push_back(a);
		dbd.push_back(DbDatum("archive_abs_change"));
		dbd.push_back(DbDatum("archive_rel_change"));
		dbd.push_back(DbDatum("archive_period"));
		dba.delete_property(dbd);

		dbd.clear();
		a << (short)1;
		dbd.push_back(a);
		dbd.push_back(ch);
		dba.put_property(dbd);

		DeviceProxy adm_dev_3(device->adm_name().c_str());
		di << device_name;
		adm_dev_3.command_inout("DevRestart",di);

		delete device;
		device = new DeviceProxy(device_name);
		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// subscribe to a archive event with a filter on delta_event
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;
		filters.clear();
		filters.push_back("$delta_event >= 500.0 ");

		// start the polling first!
		device->poll_attribute(att_name,250);
#ifdef NOTIFD
		eve_id = device->subscribe_event(att_name,Tango::ARCHIVE_EVENT,&cb,filters);

//
// Check that the attribute is still polled at 1000 mS
//

		po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 250);

		TEST_LOG << "   subscribe_event (with filter on delta_event) --> OK" << std::endl;

//
// Check that first point has been received
//

		assert (cb.cb_executed == 1);
		assert (cb.val == 30);
		assert (cb.val_size == 4);
		TEST_LOG << "   first point received (with delta_event filter) --> OK" << std::endl;

		device->command_inout("IOIncValue");

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

//
// Generates a positive change
//

		device->command_inout("IOIncValue");
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		device->command_inout("IOIncValue");
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		device->command_inout("IOIncValue");
		std::this_thread::sleep_for(std::chrono::milliseconds(700));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert ( cb.cb_executed == 1);
		assert (cb.val == 30);
		assert (cb.val_size == 4);

		TEST_LOG << "   Event filtered with delta_event filter set --> OK" << std::endl;

//
// Change once more the value and wait for the event

		device->command_inout("IOIncValue");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 2);
		assert (cb.val == 35);
		assert (cb.val_size == 4);

		TEST_LOG << "   Event received with delta_event filter set --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);
		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;
#endif

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

	return 0;
}
