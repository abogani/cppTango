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
	catch (...)
	{
		TEST_LOG << "EventCallBack::push_event(): could not extract data !\n";
	}

}


class EventUnsubCallBack : public Tango::CallBack
{
public:
	EventUnsubCallBack(DeviceProxy *d):Tango::CallBack(),cb_executed(0),dev(d) {}

	void push_event(Tango::EventData*);
	void set_ev_id(int e) {ev_id = e;}

protected:
	int 				cb_executed;
	int 				ev_id;
	Tango::DeviceProxy 	*dev;
};

void EventUnsubCallBack::push_event(Tango::EventData* event_data)
{
	TEST_LOG << "EventUnsubCallBack::push_event() for attribute " << event_data->attr_name << std::endl;
	cb_executed++;
	if (cb_executed == 2)
	{
		dev->unsubscribe_event(ev_id);
	}
}

//---------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	// TODO: FIXME: This test is temporarily disabled due to failures on single-core machines.
	return 0;

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
// Check that callback was called
//

		std::this_thread::sleep_for(std::chrono::seconds(3));
		TEST_LOG << "Callback cb_err = " << cb.cb_err << std::endl;
		assert (cb.cb_err == 1);

		TEST_LOG << "   CallBack executed when attribute throw exception (only once) --> OK" << std::endl;

//
// Attribute does not send exception any more
//

		data_in[1] = 0;
		di << data_in;

		device->command_inout("IOAttrThrowEx",di);

//
// Check that the event is still received event after a try to subscribe with a null callback
//

		EventCallBack cb2;
		cb2.cb_executed = 0;
		cb2.cb_err = 0;
		cb2.old_sec = cb2.old_usec = 0;

		bool ex = false;

		try
		{
			device->subscribe_event(att_name,Tango::CHANGE_EVENT,(CallBack *)NULL,filters);
		}
		catch (Tango::DevFailed &e)
		{
//			Tango::Except::print_exception(e);
			ex = true;
			std::string de(e.errors[0].desc);
		}

		device->command_inout("IOIncValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 8);
		assert (ex == true);

		TEST_LOG << "   CallBack executed after a try to subscribe to one attribute with a NULL callback --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

//
// Subscribe to the event using the full device name
//

		char *tg_host = getenv("TANGO_HOST");
		if (tg_host == NULL)
			assert(false);

		std::string full_device_name("tango://");
		full_device_name = full_device_name + tg_host + '/';
		full_device_name = full_device_name + device_name;
		Tango::DeviceProxy full_dp(full_device_name);

		cb.cb_executed = 0;
		int ev_id_full = full_dp.subscribe_event(att_name,Tango::CHANGE_EVENT,&cb,filters);

		assert (cb.cb_executed == 1);

		TEST_LOG << "   Subscription using Tango full device name --> OK" << std::endl;

		full_dp.unsubscribe_event(ev_id_full);

//
// Subscribe to the event using the full device name with fqdn
//

		std::string tg(tg_host);
		std::string::size_type pos = tg.find(':');
		if (pos == std::string::npos)
			assert(false);
		std::string host = tg.substr(0,pos);
		std::string port = tg.substr(pos);

		std::string full_device_name_fqdn("tango://");
		//TODO get host fqdn name
		full_device_name_fqdn = full_device_name_fqdn + host /*+ ".esrf.fr"*/ + port + '/';
		full_device_name_fqdn = full_device_name_fqdn + device_name;
		Tango::DeviceProxy full_dp_fq(full_device_name_fqdn);

		cb.cb_executed = 0;
		int ev_id_full_fq = full_dp_fq.subscribe_event(att_name,Tango::CHANGE_EVENT,&cb,filters);

		assert (cb.cb_executed == 1);

		TEST_LOG << "   Subscription using Tango full device name with FQDN --> OK" << std::endl;

		full_dp_fq.unsubscribe_event(ev_id_full_fq);

//sleep(1000);
delete device;
Tango::ApiUtil::cleanup();
device = new DeviceProxy(device_name);

//
// Change the event parameters. This means restart the device to take
// this into account
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		dbd.clear();
		a << (short)2;
		dbd.push_back(a);
		dbd.push_back(DbDatum("abs_change"));
		dbd.push_back(DbDatum("rel_change"));
		dba.delete_property(dbd);

		dbd.clear();
		a << (short)1;
		dbd.push_back(a);
		DbDatum drel("rel_change");
		drel << (short)10;
		dbd.push_back(drel);
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
// subscribe to a change event
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,&cb,filters);

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
		assert (cb.cb_executed == 2);

		device->command_inout("IOIncValue");
		device->command_inout("IOIncValue");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (cb.cb_executed == 3);
		assert (cb.val == 33);
		assert (cb.val_size == 4);

		TEST_LOG << "   CallBack executed for positive relative delta --> OK" << std::endl;

//
// Generates a negative change (10 % of 33 is 3.3)
//

		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		TEST_LOG << "cb excuted = " << cb.cb_executed << std::endl;
		assert (cb.cb_executed == 3);

		device->command_inout("IODecValue");
		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (cb.cb_executed == 3);

		device->command_inout("IODecValue");

		std::this_thread::sleep_for(std::chrono::seconds(1));

		assert (cb.cb_executed == 4);
		assert (cb.val == 29);
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
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   unsubscribe_event --> OK" << std::endl;

delete device;
Tango::ApiUtil::cleanup();
device = new DeviceProxy(device_name);

//
// Change the event parameters. This means restart the device to take
// this into account
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		dbd.clear();
		a << (short)2;
		dbd.push_back(a);
		dbd.push_back(DbDatum("abs_change"));
		dbd.push_back(DbDatum("rel_change"));
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
// subscribe to a change event with a filter
//

		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;
		// start the polling first!
		device->poll_attribute(att_name,1000);
#ifdef NOTIFD
		filters.push_back("$delta_change_abs >= 2 or $delta_change_abs <= -2");
		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,&cb,filters);

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

		std::this_thread::sleep_for(std::chrono::seconds(3));

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
// Try to unsubscribe within the callback
//

		EventUnsubCallBack cb_unsub(device);

		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,&cb_unsub,filters);
		cb_unsub.set_ev_id(eve_id);

		std::this_thread::sleep_for(std::chrono::seconds(2));

		device->command_inout("IOIncValue");

		std::this_thread::sleep_for(std::chrono::seconds(2));

		bool unsub = false;
		try
		{
			device->unsubscribe_event(eve_id);
		}
		catch(Tango::DevFailed &e)
		{
//			Tango::Except::print_exception(e);
			std::string reason(e.errors[0].reason);
			if (reason == API_EventNotFound)
				unsub = true;
		}

		assert (unsub == true);

		TEST_LOG << "   Event unsubscription within the callback --> OK" << std::endl;

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
