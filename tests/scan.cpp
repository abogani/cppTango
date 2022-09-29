#include "common.h"

class SlowCallBack : public Tango::CallBack
{
	void push_event(Tango::EventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;
	short val;
	AttrQuality qua;
};

class FastCallBack : public Tango::CallBack
{
	void push_event(Tango::EventData*);

public:
	int cb_executed;
	int cb_err;
	int old_sec,old_usec;
	double val;
	AttrQuality qua;
};

void SlowCallBack::push_event(Tango::EventData* event_data)
{
	std::vector<short> value;
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
		TEST_LOG << "SlowCallBack::push_event(): called attribute " << event_data->attr_name << " event " << event_data->event << "\n";
		qua = event_data->attr_value->get_quality();
		if ((!event_data->err) && (qua != ATTR_INVALID))
		{

			*(event_data->attr_value) >> value;
			val = value[0];
			TEST_LOG << "Callback value " << val << std::endl;
		}
		else
		{
			TEST_LOG << "Error send to callback" << std::endl;
//			Tango::Except::print_error_stack(event_data->errors);
		}
	}
	catch (...)
	{
		TEST_LOG << "SlowCallBack::push_event(): could not extract data !\n";
	}

}

void FastCallBack::push_event(Tango::EventData* event_data)
{
	std::vector<double> value;
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
		TEST_LOG << "FastCallBack::push_event(): called attribute " << event_data->attr_name << " event " << event_data->event << "\n";
		qua = event_data->attr_value->get_quality();
		if ((!event_data->err) && (qua != ATTR_INVALID))
		{

			*(event_data->attr_value) >> value;
			val = value[0];
			TEST_LOG << "Callback value " << val << std::endl;
		}
		else
		{
			TEST_LOG << "Error send to callback" << std::endl;
//			Tango::Except::print_error_stack(event_data->errors);
		}
	}
	catch (...)
	{
		TEST_LOG << "FastCallBack::push_event(): could not extract data !\n";
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

		std::string att_name("slow_actuator");

//
// Test set up (stop polling and clear abs_change and rel_change attribute
// properties but restart device to take this into account)
// Set the abs_change to 1
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		{
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
		}

		delete device;
		device = new DeviceProxy(device_name);
		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// switch on the polling first!
//
		device->poll_attribute(att_name,250);

//
// Check that the attribute is now polled at 1000 mS
//

		bool po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		int poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 250);

//
// subscribe to a change event and filter only on a quality change
//

		int eve_id;
		SlowCallBack cb;
		cb.cb_executed = 0;
		cb.cb_err = 0;
		cb.old_sec = cb.old_usec = 0;

		std::vector<std::string> filters;
		filters.push_back("$quality == 1");
		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,&cb,filters);

		std::this_thread::sleep_for(std::chrono::milliseconds(250));

//
// Check that first point has been received
//

		assert (cb.cb_executed == 1);
		assert (cb.qua == ATTR_VALID);
		TEST_LOG << "   (Slow actuator) first point received --> OK" << std::endl;
//
// Write a value into the actuator
//

		short new_val = 10;
		DeviceAttribute da(att_name,new_val);

		device->write_attribute(da);

		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// Check that the event has been received
//

		assert (cb.cb_executed == 2);
		assert (cb.qua == ATTR_CHANGING);
		TEST_LOG << "   (Slow actuator) Event quality from VALID->CHANGING --> OK" << std::endl;

//
// Read value from device which should have quality factor still set to
// CHANGING
//

		DeviceAttribute da_r;
		device->set_source(DEV);
		da_r = device->read_attribute(att_name);
		device->set_source(CACHE_DEV);

		assert (da_r.get_quality() == ATTR_CHANGING);
		TEST_LOG << "   (Slow actuator) Attribute quality still CHANGING after read --> OK" << std::endl;

//
// Wait for end of movement
//

		std::this_thread::sleep_for(std::chrono::seconds(3));

//
// Check that the event has been received
//

		assert (cb.cb_executed == 3);
		assert (cb.qua == ATTR_VALID);
		TEST_LOG << "   (Slow actuator) event quality from CHANGING->VALID --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);

		TEST_LOG << "   (Slow actuator) unsubscribe_event --> OK" << std::endl;

//---------------------------------------------------------------------------------
//
//			Now, check a fast actuator scan
//
//---------------------------------------------------------------------------------

		att_name = "fast_actuator";


//
// Test set up (stop polling and clear abs_change and rel_change attribute
// properties but restart device to take this into account)
// Set the abs_change to 1
//

		if (device->is_attribute_polled(att_name))
			device->stop_poll_attribute(att_name);
		{
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
		}

		delete device;
		device = new DeviceProxy(device_name);
		std::this_thread::sleep_for(std::chrono::seconds(1));

//
// switch on the polling first!
//
		device->poll_attribute(att_name,250);

//
// Check that the attribute is now polled at 100 mS
//
		po = device->is_attribute_polled(att_name);
		TEST_LOG << "attribute polled : " << po << std::endl;
		assert( po == true);

		poll_period = device->get_attribute_poll_period(att_name);
		TEST_LOG << "att polling period : " << poll_period << std::endl;
		assert( poll_period == 250);

//
// subscribe to a change event and filter only on a quality change
//

		FastCallBack fcb;
		fcb.cb_executed = 0;
		fcb.cb_err = 0;
		fcb.old_sec = cb.old_usec = 0;

		std::vector<std::string> ffilters;
		ffilters.push_back("$quality == 1");
		eve_id = device->subscribe_event(att_name,Tango::CHANGE_EVENT,&fcb,ffilters);

		TEST_LOG << "   (Fast actuator) subscribe_event --> OK" << std::endl;

		std::this_thread::sleep_for(std::chrono::milliseconds(250));

//
// Check that first point has been received
//

		assert (fcb.cb_executed == 1);
		assert (fcb.qua == ATTR_VALID);
		TEST_LOG << "   (Fast actuator) first point received --> OK" << std::endl;


//
// Write a value into the actuator
//

		double val = 100.333;
		DeviceAttribute fda(att_name,val);

		device->write_attribute(fda);

//  Only sleep a 100ms. The events should be fired immediately

		std::this_thread::sleep_for(std::chrono::milliseconds(250));

//
// Check that the events has been received
//

		assert (fcb.cb_executed == 3);
		assert (fcb.qua == ATTR_VALID);
		TEST_LOG << "   (Fast actuator) Two quality events received --> OK" << std::endl;

//
// unsubscribe to the event
//

		device->unsubscribe_event(eve_id);
		TEST_LOG << "   (Fast actuator) Unsubscribe_event --> OK" << std::endl;

		device->stop_poll_attribute(att_name);
		device->stop_poll_attribute("slow_actuator");

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
