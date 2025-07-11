// NOLINTBEGIN(*)

#include <tango/tango.h>
#include "DevTest.h"
#include <tango/server/tango_clock.h>

#ifdef WIN32
  #include <process.h>
#endif

#include <iterator>

void EventCallBack::push_event(Tango::EventData *event_data)
{
    short value;

    cb_executed++;
    try
    {
        TANGO_LOG << "EventCallBack::push_event(): called attribute " << event_data->attr_name;
        TANGO_LOG << " event " << event_data->event << std::endl;
        if(!event_data->err)
        {
            *(event_data->attr_value) >> value;
            TANGO_LOG << "CallBack vector value : " << value << std::endl;
        }
        else
        {
            TANGO_LOG << "Error sent to callback" << std::endl;
            //			Tango::Except::print_error_stack(event_data->errors);
            cb_err++;
        }
    }
    catch(...)
    {
        TANGO_LOG << "EventCallBack::push_event(): could not extract data !\n";
    }
}

//+----------------------------------------------------------------------------
//
// method : 		DevTest::DevTest()
//
// description : 	constructor for testing use of TANGO
//
// in : - cl : Pointer to the DeviceClass object
//      - s : Device name
//
//-----------------------------------------------------------------------------

DevTest::DevTest(Tango::DeviceClass *cl, std::string &s) :
    TANGO_BASE_CLASS(cl, s.c_str())
{
    init_device();
}

DevTest::DevTest(Tango::DeviceClass *cl, const char *s) :
    TANGO_BASE_CLASS(cl, s)
{
    init_device();
}

DevTest::DevTest(Tango::DeviceClass *cl, const char *s, const char *d) :
    TANGO_BASE_CLASS(cl, s, d)
{
    init_device();
}

DevTest::DevTest(Tango::DeviceClass *cl, const char *s, const char *d, Tango::DevState state, const char *status) :
    TANGO_BASE_CLASS(cl, s, d, state, status)
{
    init_device();
}

void DevTest::init_device()
{
    TANGO_LOG << "DevTest::DevTest() create  " << device_name << std::endl;
    DEBUG_STREAM << "Creating " << device_name << std::endl;

    Tango::Util *tg = Tango::Util::instance();

    set_state(Tango::ON);
    attr_long = 1246;
    attr_short_rw = 66;
    attr_long64_rw = 0x800000000LL;
    attr_ulong_rw = 0xC0000000L;
    attr_ulong64_rw = 0xC000000000000000LL;
    attr_state_rw = Tango::FAULT;
    PollLong_attr_num = 0;
    PollString_spec_attr_num = 0;
    attr_asyn_write_val = 42;
    attr_string_rw = Tango::string_dup("The incredible initial string!");

    Short_attr_except = false;
    if(tg->is_svr_starting() == true || tg->is_device_restarting(device_name) == true)
    {
        Short_attr_w_except = false;
    }
    Long_attr_except = false;
    io_init_wattr_except = false;
    event_change_attr_except = false;
    event_quality_attr_except = false;
    event_throw_out_of_sync = false;

    attr_sub_device_tst = false;

    attr_event_size = 4;
    attr_event[0] = 10;
    attr_event[1] = 20;
    attr_event[2] = 30;
    attr_event[3] = 40;
    attr_event[4] = 50;
    attr_event[5] = 60;
    attr_event[6] = 70;
    attr_event[7] = 80;
    attr_event[8] = 90;
    attr_event[9] = 100;

    attr_event64_size = 2;
    attr_event64[0] = 0x800000000LL;
    attr_event64[1] = 44;
    attr_event64[2] = 55;

    attr_qua_event[0] = 1.2;
    attr_qua_event[1] = 2.4;
    attr_event_qua = Tango::ATTR_VALID;

    slow_actua_write.tv_sec = 0;
    slow_actua = 0;

    attr_spec_long64_rw[0] = 1000;
    attr_spec_long64_rw[1] = 10000;
    attr_spec_long64_rw[2] = 100000;
    attr_spec_ulong_rw[0] = 2222;
    attr_spec_ulong_rw[1] = 22222;
    attr_spec_ulong_rw[2] = 222222;
    attr_spec_ulong64_rw[0] = 8888;
    attr_spec_ulong64_rw[1] = 88888;
    attr_spec_ulong64_rw[2] = 888888;
    attr_spec_state_rw[0] = Tango::ON;
    attr_spec_state_rw[1] = Tango::OFF;

    attr_slow = 3.3;

    enc_attr.encoded_format = Tango::string_dup("Which format?");
    /*  	enc_attr.encoded_data.length(200 * 1024 * 1024);
            for (int i = 0;i < (200 * 1024 * 1024);i++)
                    enc_attr.encoded_data[i] = (unsigned char)(i % 256);*/
    enc_attr.encoded_data.length(4);
    enc_attr.encoded_data[0] = (unsigned char) 97;
    enc_attr.encoded_data[1] = (unsigned char) 98;
    enc_attr.encoded_data[2] = (unsigned char) 99;
    enc_attr.encoded_data[3] = (unsigned char) 100;

    /*	enc_format = new char[40];
            strcpy(enc_format,"Forth try");
            enc_data[0] = (unsigned char)40;
            enc_data[1] = (unsigned char)41;*/

    att_conf = 10;
    wattr_throw = 0;
    enum_value = 2;

    Tango::Attribute &att = (get_device_attr())->get_attr_by_name("DynEnum_attr");
    Tango::MultiAttrProp<Tango::DevEnum> multi_prop;
    att.get_properties(multi_prop);
    if(multi_prop.enum_labels.empty() == true)
    {
        multi_prop.enum_labels.push_back("Dummy_label");
    }
    att.set_properties(multi_prop);

    rpipe_type = 0;
    Tango::Pipe &pi = get_device_class()->get_pipe_by_name("RWPipe", device_name_lower);
    pi.set_pipe_serial_model(Tango::PIPE_BY_USER);

    Reynald_ctr = 0;
    Reynald_val = 9.9;

    TANGO_LOG << "DevTest::DevTest(): End of init_device() method for device " << device_name << std::endl;
}

void DevTest::delete_device()
{
    Tango::string_free(attr_string_rw);
}

//+----------------------------------------------------------------------------
//
// method : 		DevTest::always_executed_hook()
//
// description : 	method always executed before any command is executed
//
//-----------------------------------------------------------------------------

void DevTest::always_executed_hook()
{
    TANGO_LOG_INFO << "In always_executed_hook method" << std::endl;
}

void DevTest::server_init_hook()
{
    TANGO_LOG_INFO << "In server_init_hook method" << std::endl;
}

//+----------------------------------------------------------------------------
//
// method : 		DevTest::state_cmd()
//
// description : 	command to read the device state
//
// out :		device state
//
//-----------------------------------------------------------------------------

Tango::ConstDevString DevTest::dev_status()
{
    TANGO_LOG_INFO << "In DayOfWeek state command" << std::endl;
#ifdef WIN32
    Tango::ConstDevString stat = DeviceImpl::dev_status();
#else
    Tango::ConstDevString stat = Tango::DeviceImpl::dev_status();
#endif
    return stat;
}

//+----------------------------------------------------------------------------
//
// method : 		DevTest::xxx
//
// description : 	Methods for template style command implementation
//
//-----------------------------------------------------------------------------

bool DevTest::templ_state(const CORBA::Any &)
{
    //
    // command allowed only if the device is on
    //

    if(device_state == Tango::ON)
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

void DevTest::IOTempl()
{
    TANGO_LOG << "[DevTest::IOTempl]" << std::endl;
}

Tango::DevVarLongArray *DevTest::IOTemplOut()
{
    TANGO_LOG << "[DevTest::IOTemplOut]" << std::endl;

    Tango::DevVarLongArray *argout;
    argout = new Tango::DevVarLongArray();

    argout->length(4);
    (*argout)[0] = 10;
    (*argout)[1] = 20;
    (*argout)[2] = 30;
    (*argout)[3] = 40;

    return argout;
}

void DevTest::IOTemplIn(Tango::DevLong received)
{
    TANGO_LOG << "[DevTest::IOTemplIn] received " << received << std::endl;
}

void DevTest::IOPushEvent()
{
    TANGO_LOG << "[DevTest::IOPushEvent] received " << std::endl;

    std::vector<std::string> f_names;
    std::vector<double> f_val;

    push_event("event_change_tst", f_names, f_val, attr_event, attr_event_size);
}

void DevTest::IOPushDevEncodedEvent()
{
    TANGO_LOG << "[DevTest::IOPushDevEncodedEvent] received " << std::endl;

    std::vector<std::string> f_names;
    std::vector<double> f_val;

    push_event("encoded_attr", f_names, f_val, &enc_attr);
}

Tango::DevLong DevTest::IOSubscribeEvent(const Tango::DevVarStringArray *in_data)
{
    TANGO_LOG << "[DevTest::IOSubscribeEvent] received " << std::endl;

    // TODO: modify conf_devtest.cpp to automatically create a remote device based on the instance name

    //	vector<string> filters;
    //
    //	if (remote_dev == nullptr)
    //	{
    //    	Tango::Util *tg = Tango::Util::instance();
    //		vector<Tango::DeviceImpl *> &dev_list = tg->get_device_list_by_class("DevTest");
    //		if(dev_list.size() > 1) // the second device on the list is chosen
    //		{
    //			vector<string> devices;
    //			for(size_t i = 0; i < dev_list.size(); i++)
    //				devices.push_back(dev_list[i]->get_name());
    //			sort(devices.begin(),devices.end());
    //			remote_dev = new Tango::DeviceProxy(devices[1]);
    //		}
    //		else
    //			TANGO_LOG << "EXCEPTION: at least 2 devices have to be defined" << std::endl;
    //	}
    //	string att_name("short_attr");
    //	cb.cb_executed = 0;
    //
    //	// start the polling first!
    //	remote_dev->poll_attribute(att_name,1000);
    //	eve_id = remote_dev->subscribe_event(att_name,Tango::PERIODIC_EVENT,&cb,filters);

    std::vector<std::string> filters;

    remote_dev = std::make_shared<Tango::DeviceProxy>((*in_data)[0]);
    std::string att_name((*in_data)[1]);
    cb.cb_executed = 0;

    // start the polling first!
    remote_dev->poll_attribute(att_name, 1000);

    std::string eve_type((*in_data)[2]);
    std::transform(eve_type.begin(), eve_type.end(), eve_type.begin(), ::tolower);
    Tango::EventType eve;
    if(eve_type == "change")
    {
        eve = Tango::CHANGE_EVENT;
    }
    else if(eve_type == "periodic")
    {
        eve = Tango::PERIODIC_EVENT;
    }
    else if(eve_type == "archive")
    {
        eve = Tango::ARCHIVE_EVENT;
    }
    else
    {
        std::stringstream ss;
        ss << "Event type " << (*in_data)[2] << " not recognized as a valid event type";
        TANGO_THROW_EXCEPTION("DevTest_WrongEventType", ss.str());
    }

    int eve_id = remote_dev->subscribe_event(att_name, eve, &cb, filters);

    event_atts.insert(make_pair(eve_id, att_name));

    return eve_id;
}

void DevTest::IOUnSubscribeEvent(Tango::DevLong &in_data)
{
    TANGO_LOG << "[DevTest::IOUnSubscribeEvent] received " << std::endl;

    if(in_data != 0)
    {
        std::map<int, std::string>::iterator ite = event_atts.find(in_data);
        if(ite == event_atts.end())
        {
            TANGO_THROW_EXCEPTION("DevTest_WrongEventID", "Cant find event id in map");
        }
        remote_dev->unsubscribe_event(in_data);

        remote_dev->stop_poll_attribute(ite->second);
    }
}

void DevTest::IOFillPollBuffAttr()
{
    TANGO_LOG << "[DevTest::IOFillPollBuffAttr] received " << std::endl;

    Tango::AttrHistoryStack<Tango::DevString> ahs;
    ahs.length(3);
    Tango::Util *tg = Tango::Util::instance();
    Tango::DevString *main_array[3];
    std::string att_name("Poll_buff");

    Tango::DevString *array_1 = new Tango::DevString[4];
    //     	array_1[0] = Tango::string_dup("One_1");
    //      array_1[1] = Tango::string_dup("Two_1");
    //      array_1[2] = Tango::string_dup("Three_1");
    //      array_1[3] = Tango::string_dup("Four_1");
    array_1[0] = Tango::string_dup("One_1");
    array_1[1] = Tango::string_dup("Two_1");
    array_1[2] = Tango::string_dup("Three_1");
    array_1[3] = Tango::string_dup("Four_1");
    main_array[0] = array_1;

    Tango::DevString *array_2 = new Tango::DevString[4];
    array_2[0] = Tango::string_dup("One_2");
    array_2[1] = Tango::string_dup("Two_2");
    array_2[2] = Tango::string_dup("Three_2");
    array_2[3] = Tango::string_dup("Four_2");
    main_array[1] = array_2;

    Tango::DevString *array_3 = new Tango::DevString[4];
    array_3[0] = Tango::string_dup("One_3");
    array_3[1] = Tango::string_dup("Two_3");
    array_3[2] = Tango::string_dup("Three_3");
    array_3[3] = Tango::string_dup("Four_3");
    main_array[2] = array_3;

    ahs.clear();
    time_t when = Tango::get_current_system_datetime();
    for(int k = 0; k < 3; k++)
    {
        Tango::TimedAttrData<Tango::DevString> tad(main_array[k], 2, 2, Tango::ATTR_VALID, true, when);
        ahs.push(tad);
    }

    tg->fill_attr_polling_buffer(this, att_name, ahs);

    ahs.clear();
    att_name = "Poll_buffRW";
    when = Tango::get_current_system_datetime();

    Tango::DevString *rd_array_1 = new Tango::DevString[4];
    Tango::DevString *wr_array_1 = new Tango::DevString[2];

    rd_array_1[0] = Tango::string_dup("One_rd_1");
    rd_array_1[1] = Tango::string_dup("Two_rd_1");
    rd_array_1[2] = Tango::string_dup("Three_rd_1");
    rd_array_1[3] = Tango::string_dup("Four_rd_1");

    wr_array_1[0] = Tango::string_dup("One_wr_1");
    wr_array_1[1] = Tango::string_dup("Two_wr_1");

    Tango::TimedAttrData<Tango::DevString> tad_1(rd_array_1, 2, 2, wr_array_1, 2, 1, Tango::ATTR_VALID, true, when);
    ahs.push(tad_1);

    Tango::DevString *rd_array_2 = new Tango::DevString[4];
    Tango::DevString *wr_array_2 = new Tango::DevString[2];

    rd_array_2[0] = Tango::string_dup("One_rd_2");
    rd_array_2[1] = Tango::string_dup("Two_rd_2");
    rd_array_2[2] = Tango::string_dup("Three_rd_2");
    rd_array_2[3] = Tango::string_dup("Four_rd_2");

    wr_array_2[0] = Tango::string_dup("One_wr_2");
    wr_array_2[1] = Tango::string_dup("Two_wr_2");

    Tango::TimedAttrData<Tango::DevString> tad_2(rd_array_2, 2, 2, wr_array_2, 2, 1, Tango::ATTR_VALID, true, when);
    ahs.push(tad_2);

    Tango::DevString *rd_array_3 = new Tango::DevString[4];
    Tango::DevString *wr_array_3 = new Tango::DevString[2];

    rd_array_3[0] = Tango::string_dup("One_rd_3");
    rd_array_3[1] = Tango::string_dup("Two_rd_3");
    rd_array_3[2] = Tango::string_dup("Three_rd_3");
    rd_array_3[3] = Tango::string_dup("Four_rd_3");

    wr_array_3[0] = Tango::string_dup("One_wr_3");
    wr_array_3[1] = Tango::string_dup("Two_wr_3");

    Tango::TimedAttrData<Tango::DevString> tad_3(rd_array_3, 2, 2, wr_array_3, 2, 1, Tango::ATTR_VALID, true, when);
    ahs.push(tad_3);

    tg->fill_attr_polling_buffer(this, att_name, ahs);

    TANGO_LOG << "Attribute Polling buffer filled" << std::endl;
}

void DevTest::IOFillPollBuffEncodedAttr()
{
    TANGO_LOG << "[DevTest::IOFillPollBuffEncodedAttr] received " << std::endl;

    Tango::AttrHistoryStack<Tango::DevEncoded> ahs;
    ahs.length(3);
    std::string att_name("Encoded_attr");
    Tango::Util *tg = Tango::Util::instance();
    Tango::DevEncoded the_enc;
    Tango::DevEncoded the_enc1;
    Tango::DevEncoded the_enc2;

    the_enc.encoded_format = Tango::string_dup("First value");
    the_enc.encoded_data.length(2);
    the_enc.encoded_data[0] = 22;
    the_enc.encoded_data[1] = 33;

    time_t when = Tango::get_current_system_datetime();
    Tango::TimedAttrData<Tango::DevEncoded> tad(&the_enc, when);
    ahs.push(tad);

    the_enc1.encoded_format = Tango::string_dup("Second value");
    the_enc1.encoded_data.length(2);
    the_enc1.encoded_data[0] = 33;
    the_enc1.encoded_data[1] = 44;

    when = Tango::get_current_system_datetime();
    Tango::TimedAttrData<Tango::DevEncoded> tad1(&the_enc1, when);
    ahs.push(tad1);

    the_enc2.encoded_format = Tango::string_dup("Third value");
    the_enc2.encoded_data.length(2);
    the_enc2.encoded_data[0] = 44;
    the_enc2.encoded_data[1] = 55;

    when = Tango::get_current_system_datetime();
    Tango::TimedAttrData<Tango::DevEncoded> tad2(&the_enc2, when);
    ahs.push(tad2);

    tg->fill_attr_polling_buffer(this, att_name, ahs);
    TANGO_LOG << "Attribute (DevEncoded data type) polling buffer filled" << std::endl;
}

void DevTest::IOFillPollBuffCmd()
{
    TANGO_LOG << "[DevTest::IOFillPollBuffCmd] received " << std::endl;

    Tango::CmdHistoryStack<Tango::DevVarLongArray> chs;
    chs.length(3);
    Tango::Util *tg = Tango::Util::instance();

    Tango::DevVarLongArray dvla_array[10];
    std::string cmd_name("IOArray1");

    for(int j = 0; j < 10; j++)
    {
        dvla_array[j].length(3);
        dvla_array[j][0] = 10 + j;
        dvla_array[j][1] = 11 + j;
        dvla_array[j][2] = 12 + j;
    }

    chs.clear();
    time_t when = Tango::get_current_system_datetime();
    for(int k = 0; k < 3; k++)
    {
        Tango::TimedCmdData<Tango::DevVarLongArray> tad(&dvla_array[k], when);
        chs.push(tad);
    }

    tg->fill_cmd_polling_buffer(this, cmd_name, chs);

    TANGO_LOG << "Command Polling buffer filled" << std::endl;
}

Tango::DevVarDoubleArray *DevTest::IOTemplInOut(Tango::DevDouble db)
{
    TANGO_LOG << "[DevTest::IOTemplInOut] received " << db << std::endl;

    Tango::DevVarDoubleArray *argout;
    argout = new Tango::DevVarDoubleArray();

    argout->length(2);
    (*argout)[0] = db;
    (*argout)[1] = db * 2;

    return argout;
}

void DevTest::IOSetWAttrLimit(const Tango::DevVarDoubleArray *in)
{
    Tango::WAttribute &w_attr = dev_attr->get_w_attr_by_name("Double_attr_w");
    Tango::DevDouble limit = (*in)[1];
    if((*in)[0] == 0.0)
    {
        w_attr.set_min_value(limit);
    }
    else
    {
        w_attr.set_max_value(limit);
    }
}

void DevTest::push_data_ready(const Tango::DevVarLongStringArray *in)
{
    TANGO_LOG << "Pushing Data Ready event for attribute " << in->svalue[0].in() << std::endl;
    push_data_ready_event(in->svalue[0].in(), in->lvalue[0]);
}

Tango::DevVarStringArray *DevTest::IOPollingInDevice()
{
    Tango::DevVarStringArray *ret = new Tango::DevVarStringArray();
    ret->length(12);
    std::stringstream ss;
    std::string att_name("Double_spec_attr");
    std::string cmd_name("OULong");

    // is_xxx_polled

    ss << "Attribute " << att_name << " polled = ";
    if(is_attribute_polled(att_name) == true)
    {
        ss << "true";
    }
    else
    {
        ss << "false";
    }
    (*ret)[0] = Tango::string_dup(ss.str().c_str());

    ss.str("");
    ss.clear();
    ss << "Command " << cmd_name << " polled = ";
    if(is_command_polled(cmd_name) == true)
    {
        ss << "true";
    }
    else
    {
        ss << "false";
    }
    (*ret)[1] = Tango::string_dup(ss.str().c_str());

    // get_xxx_poll_period

    ss.str("");
    ss.clear();
    ss << "Attribute " << att_name << " polling period = " << get_attribute_poll_period(att_name);
    (*ret)[2] = Tango::string_dup(ss.str().c_str());

    ss.str("");
    ss.clear();
    ss << "Command " << cmd_name << " polling period = " << get_command_poll_period(cmd_name);
    (*ret)[3] = Tango::string_dup(ss.str().c_str());

    // poll_xxx

    poll_attribute(att_name, 250);
    poll_command(cmd_name, 250);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // is_xxx_polled

    ss.str("");
    ss.clear();
    ss << "Attribute " << att_name << " polled = ";
    if(is_attribute_polled(att_name) == true)
    {
        ss << "true";
    }
    else
    {
        ss << "false";
    }
    (*ret)[4] = Tango::string_dup(ss.str().c_str());

    ss.str("");
    ss.clear();
    ss << "Command " << cmd_name << " polled = ";
    if(is_command_polled(cmd_name) == true)
    {
        ss << "true";
    }
    else
    {
        ss << "false";
    }
    (*ret)[5] = Tango::string_dup(ss.str().c_str());

    // get_xxx_poll_period

    ss.str("");
    ss.clear();
    ss << "Attribute " << att_name << " polling period = " << get_attribute_poll_period(att_name);
    (*ret)[6] = Tango::string_dup(ss.str().c_str());

    ss.str("");
    ss.clear();
    ss << "Command " << cmd_name << " polling period = " << get_command_poll_period(cmd_name);
    (*ret)[7] = Tango::string_dup(ss.str().c_str());

    // stop_poll_xxx

    stop_poll_attribute(att_name);
    stop_poll_command(cmd_name);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // is_xxx_polled

    ss.str("");
    ss.clear();
    ss << "Attribute " << att_name << " polled = ";
    if(is_attribute_polled(att_name) == true)
    {
        ss << "true";
    }
    else
    {
        ss << "false";
    }
    (*ret)[8] = Tango::string_dup(ss.str().c_str());

    ss.str("");
    ss.clear();
    ss << "Command " << cmd_name << " polled = ";
    if(is_command_polled(cmd_name) == true)
    {
        ss << "true";
    }
    else
    {
        ss << "false";
    }
    (*ret)[9] = Tango::string_dup(ss.str().c_str());

    // get_xxx_poll_period

    ss.str("");
    ss.clear();
    ss << "Attribute " << att_name << " polling period = " << get_attribute_poll_period(att_name);
    (*ret)[10] = Tango::string_dup(ss.str().c_str());

    ss.str("");
    ss.clear();
    ss << "Command " << cmd_name << " polling period = " << get_command_poll_period(cmd_name);
    (*ret)[11] = Tango::string_dup(ss.str().c_str());

    return ret;
}

void DevTest::IOSophisticatedPollInDevice()
{
    std::string att1_name("Double_spec_attr");
    std::string att2_name("Short_attr");
    std::string att3_name("ReynaldPollAttr");

    // poll the 3 atts

    poll_attribute(att1_name, 500);
    poll_attribute(att2_name, 250);
    poll_attribute(att3_name, 250);
}

Tango::DevVarStringArray *DevTest::IOGetPollMess()
{
    Tango::DevVarStringArray *dvsa = new Tango::DevVarStringArray();
    dvsa->length(poll_messages.size());

    for(size_t loop = 0; loop < poll_messages.size(); loop++)
    {
        (*dvsa)[loop] = Tango::string_dup(poll_messages[loop].c_str());
    }

    poll_messages.clear();

    return dvsa;
}

void DevTest::set_enum_labels()
{
    Tango::Attribute &att = (get_device_attr())->get_attr_by_name("DynEnum_attr");
    Tango::MultiAttrProp<Tango::DevEnum> multi_prop;
    att.get_properties(multi_prop);

    std::vector<std::string> v_s;
    v_s.push_back("Zero");
    v_s.push_back("One");
    v_s.push_back("Two");
    v_s.push_back("Three");
    multi_prop.enum_labels = v_s;
    att.set_properties(multi_prop);
}

void DevTest::add_enum_label(Tango::DevString new_label)
{
    Tango::Attribute &att = (get_device_attr())->get_attr_by_name("DynEnum_attr");
    Tango::MultiAttrProp<Tango::DevEnum> multi_prop;
    att.get_properties(multi_prop);
    multi_prop.enum_labels.push_back(new_label);
    att.set_properties(multi_prop);
}

//+----------------------------------------------------------------------------
//
// method : 		DevTest::signal_handler()
//
// description : 	command to read the device state
//
// out :		device state
//
//-----------------------------------------------------------------------------

void DevTest::signal_handler(long signo)
{
    TANGO_LOG << "[Device signal handler] received signal number " << signo;
    TANGO_LOG << " for device " << device_name << std::endl;

    DEBUG_STREAM << "[Device signal handler] received signal number " << signo << " for device " << device_name
                 << std::endl;
}

//+----------------------------------------------------------------------------
//
// method : 		DevTest::write_xxx
//
// description : 	Write attribute methods
//
//-----------------------------------------------------------------------------

void DevTest::write_Short_attr_w(Tango::WAttribute &att)
{
    //	TANGO_LOG << "In write_Short_attr_w for attribute " << att.get_name() << std::endl;

    Tango::DevShort sh;
    att.get_write_value(sh);
    //	TANGO_LOG << "Attribute value = " << sh << std::endl;
    if(Short_attr_w_except == true)
    {
        TANGO_THROW_EXCEPTION("Aaaa", "Bbbb");
    }
}

void DevTest::write_Short_attr_w2(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Short_attr_w2 for attribute " << att.get_name() << std::endl;

    Tango::DevShort sh;
    att.get_write_value(sh);
    TANGO_LOG << "Attribute value = " << sh << std::endl;
}

void DevTest::write_Long_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Long_attr_w for attribute " << att.get_name() << std::endl;

    Tango::DevLong lg;
    att.get_write_value(lg);
    TANGO_LOG << "Attribute value = " << lg << std::endl;
}

void DevTest::write_Double_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Double_attr_w for attribute " << att.get_name() << std::endl;

    Tango::DevDouble db;
    att.get_write_value(db);
    TANGO_LOG << "Attribute value = " << db << std::endl;
}

void DevTest::write_String_attr_w(Tango::WAttribute &att)
{
    //	TANGO_LOG << "In write_String_attr_w for attribute " << att.get_name() << std::endl;

    Tango::DevString str;
    att.get_write_value(str);
    //	TANGO_LOG << "Attribute value = " << str << std::endl;
}

void DevTest::write_String_attr_w2(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_String_attr_w2 for attribute " << att.get_name() << std::endl;

    Tango::DevString str;
    att.get_write_value(str);
    TANGO_LOG << "Attribute value = " << str << std::endl;
}

void DevTest::write_attr_asyn_write(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_attr_asyn_write for attribute " << att.get_name() << std::endl;

    Tango::DevLong lg;
    att.get_write_value(lg);
    attr_asyn_write_val = lg;
    TANGO_LOG << "Attribute value = " << lg << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void DevTest::read_attr_asyn_write(Tango::Attribute &att)
{
    TANGO_LOG << "In read_attr_asyn_write for attribute " << att.get_name() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    att.set_value(&attr_asyn_write_val);
}

void DevTest::write_attr_asyn_write_to(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_attr_asyn_write_to for attribute " << att.get_name() << std::endl;

    Tango::DevLong lg;
    att.get_write_value(lg);
    TANGO_LOG << "Attribute value = " << lg << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(4));
}

void DevTest::write_attr_asyn_write_except(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_attr_asyn_write_except for attribute " << att.get_name() << std::endl;

    Tango::DevLong lg;
    att.get_write_value(lg);
    TANGO_LOG << "Attribute value = " << lg << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    TANGO_THROW_EXCEPTION("aaa", "This is a test");
}

void DevTest::write_String_spec_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_String_spec_attr_w for attribute " << att.get_name() << std::endl;

    const Tango::ConstDevString *ptr;
    att.get_write_value(ptr);
    long nb_str = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_str << std::endl;
    for(int k = 0; k < nb_str; k++)
    {
        TANGO_LOG << ptr[k] << std::endl;
    }
}

void DevTest::write_Short_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Short_attr_rw for attribute " << att.get_name() << std::endl;
    att.get_write_value(attr_short_rw);
}

void DevTest::write_Long64_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Long64_attr_rw for attribute " << att.get_name() << std::endl;
    att.get_write_value(attr_long64_rw);
}

void DevTest::write_ULong_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_ULong_attr_rw for attribute " << att.get_name() << std::endl;
    att.get_write_value(attr_ulong_rw);

    if(attr_ulong_rw > 1000)
    {
        att.set_write_value((Tango::DevULong) 1111);
    }
}

void DevTest::write_ULong64_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_ULong64_attr_rw for attribute " << att.get_name() << std::endl;
    att.get_write_value(attr_ulong64_rw);
}

void DevTest::write_State_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_State_attr_rw for attribute " << att.get_name() << std::endl;
    att.get_write_value(attr_state_rw);
}

void DevTest::write_String_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_String_attr_rw for attribute " << att.get_name() << std::endl;
    Tango::DevString str;
    att.get_write_value(str);
    Tango::string_free(attr_string_rw);
    attr_string_rw = Tango::string_dup(str);
}

void DevTest::write_Float_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Float_attr_w for attribute " << att.get_name() << std::endl;

    Tango::DevFloat fl;
    att.get_write_value(fl);
    TANGO_LOG << "Attribute value = " << fl << std::endl;
}

void DevTest::write_Boolean_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Boolean_attr_w for attribute " << att.get_name() << std::endl;

    Tango::DevBoolean bo;
    att.get_write_value(bo);
    TANGO_LOG << "Attribute value = " << bo << std::endl;
}

void DevTest::write_UShort_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_UShort_attr_w for attribute " << att.get_name() << std::endl;

    Tango::DevUShort ush;
    att.get_write_value(ush);
    TANGO_LOG << "Attribute value = " << ush << std::endl;
}

void DevTest::write_UChar_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_UChar_attr_w for attribute " << att.get_name() << std::endl;

    Tango::DevUChar uch;
    att.get_write_value(uch);
    TANGO_LOG << "Attribute value = " << uch << std::endl;
}

void DevTest::write_Float_spec_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Float_spec_attr_w for attribute " << att.get_name() << std::endl;

    const Tango::DevFloat *fl;
    att.get_write_value(fl);
    long nb_fl = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_fl << std::endl;
    for(int k = 0; k < nb_fl; k++)
    {
        TANGO_LOG << fl[k] << std::endl;
    }
}

void DevTest::write_Boolean_spec_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Boolean_spec_attr_w for attribute " << att.get_name() << std::endl;

    const Tango::DevBoolean *bo;
    att.get_write_value(bo);
    long nb_bo = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_bo << std::endl;
    for(int k = 0; k < nb_bo; k++)
    {
        TANGO_LOG << bo[k] << std::endl;
    }
}

void DevTest::write_UShort_spec_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_UShort_spec_attr_w for attribute " << att.get_name() << std::endl;

    const Tango::DevUShort *ush;
    att.get_write_value(ush);
    long nb_ush = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_ush << std::endl;
    for(int k = 0; k < nb_ush; k++)
    {
        TANGO_LOG << ush[k] << std::endl;
    }
}

void DevTest::write_UChar_spec_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_UChar_spec_attr_w for attribute " << att.get_name() << std::endl;

    const Tango::DevUChar *uch;
    att.get_write_value(uch);
    long nb_uch = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_uch << std::endl;
    //	for (int k = 0;k < nb_uch;k++)
    //		TANGO_LOG << uch[k] << std::endl;
}

void DevTest::write_Float_ima_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Float_ima_attr_w for attribute " << att.get_name() << std::endl;

    const Tango::DevFloat *fl;
    att.get_write_value(fl);
    long nb_fl = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_fl << std::endl;
    //	for (int k = 0;k < nb_fl;k++)
    //		TANGO_LOG << fl[k] << std::endl;
}

void DevTest::write_UShort_ima_attr_w(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_UShort_ima_attr_w for attribute " << att.get_name() << std::endl;

    const Tango::DevUShort *ush;
    att.get_write_value(ush);
    long nb_ush = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_ush << std::endl;
    //	for (int k = 0;k < nb_ush;k++)
    //		TANGO_LOG << ush[k] << std::endl;
}

void DevTest::write_Float_spec_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Float_spec_attr_rw for attribute " << att.get_name() << std::endl;

    const Tango::DevFloat *fl;
    att.get_write_value(fl);
    long nb_fl = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_fl << std::endl;
    //	for (int k = 0;k < nb_fl;k++)
    //		TANGO_LOG << fl[k] << std::endl;
}

void DevTest::write_Long_spec_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Long_spec_attr_rw for attribute " << att.get_name() << std::endl;

    const Tango::DevLong *fl;
    att.get_write_value(fl);
    long nb_fl = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_fl << std::endl;
    for(int k = 0; k < nb_fl; k++)
    {
        TANGO_LOG << fl[k] << std::endl;
    }
}

void DevTest::write_UChar_spec_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_UShort_spec_attr_rw for attribute " << att.get_name() << std::endl;

    const Tango::DevUChar *uch;
    att.get_write_value(uch);
    long nb_uch = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_uch << std::endl;
    //	for (int k = 0;k < nb_uch;k++)
    //		TANGO_LOG << uch[k] << std::endl;
}

void DevTest::write_Boolean_ima_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Boolean_ima_attr_rw for attribute " << att.get_name() << std::endl;

    const Tango::DevBoolean *bo;
    att.get_write_value(bo);
    long nb_bo = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_bo << std::endl;
    //	for (int k = 0;k < nb_bo;k++)
    //		TANGO_LOG << bo[k] << std::endl;
}

void DevTest::write_UShort_ima_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_UShort_ima_attr_rw for attribute " << att.get_name() << std::endl;

    const Tango::DevUShort *ush;
    att.get_write_value(ush);
    long nb_ush = att.get_write_value_length();
    TANGO_LOG << ", Elt nb = " << nb_ush << std::endl;
    //	for (int k = 0;k < nb_ush;k++)
    //		TANGO_LOG << ush[k] << std::endl;
}

void DevTest::write_slow_actuator(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_slow_actuator for attribute " << att.get_name() << std::endl;

    att.get_write_value(slow_actua);
    slow_actua_write = Tango::make_timeval(std::chrono::system_clock::now());
}

void DevTest::write_fast_actuator(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_fast_actuator for attribute " << att.get_name() << std::endl;

    att.get_write_value(fast_actua);

    att.set_value(&fast_actua);
    att.set_quality(Tango::ATTR_CHANGING, true);
    att.set_value(&fast_actua);
    att.set_quality(Tango::ATTR_VALID, true);
}

void DevTest::write_Long64_spec_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Long64_spec_attr_rw for attribute " << att.get_name() << std::endl;
    const Tango::DevLong64 *lg64;
    long nb_data = att.get_write_value_length();
    att.get_write_value(lg64);
    TANGO_LOG << "Received " << nb_data << " for attribute " << att.get_name() << std::endl;
}

void DevTest::write_ULong_spec_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_ULong_spec_attr_rw for attribute " << att.get_name() << std::endl;
    const Tango::DevULong *ulg;
    long nb_data = att.get_write_value_length();
    att.get_write_value(ulg);
    TANGO_LOG << "Received " << nb_data << " for attribute " << att.get_name() << std::endl;
}

void DevTest::write_ULong64_spec_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_ULong64_spec_attr_rw for attribute " << att.get_name() << std::endl;
    const Tango::DevULong64 *ulg64;
    long nb_data = att.get_write_value_length();
    att.get_write_value(ulg64);
    TANGO_LOG << "Received " << nb_data << " for attribute " << att.get_name() << std::endl;
}

void DevTest::write_State_spec_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_State_aspec_ttr_rw for attribute " << att.get_name() << std::endl;
    const Tango::DevState *sta;
    long nb_data = att.get_write_value_length();
    att.get_write_value(sta);
    TANGO_LOG << "Received " << nb_data << " for attribute " << att.get_name() << std::endl;
}

void DevTest::write_Encoded_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Encoded_attr_rw for attribute " << att.get_name() << std::endl;
    const Tango::DevEncoded *enc;
    att.get_write_value(enc);
    TANGO_LOG << "\tReceived string = " << enc->encoded_format << std::endl;
    TANGO_LOG << "\tReceived data nb = " << enc->encoded_data.length() << std::endl;
    for(unsigned int i = 0; i < enc->encoded_data.length(); i++)
    {
        TANGO_LOG << "\t\tdata[" << i << "] = " << (unsigned long) enc->encoded_data[i] << std::endl;
    }
}

void DevTest::write_Poll_buffRW(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Poll_buffRW for attribute " << att.get_name() << std::endl;
}

void DevTest::write_Def_attr(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Def_attr for attribute " << att.get_name() << std::endl;
}

void DevTest::write_DefUser_attr(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_DefUser_attr for attribute " << att.get_name() << std::endl;
}

void DevTest::write_DefClass_attr(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_DefClass_attr for attribute " << att.get_name() << std::endl;
}

void DevTest::write_DefClassUser_attr(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_DefClassUser_attr for attribute " << att.get_name() << std::endl;
}

void DevTest::write_Enum_attr(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Enum_attr for attribute " << att.get_name() << std::endl;
}

void DevTest::write_Enum_spec_attr_rw(Tango::WAttribute &att)
{
    TANGO_LOG << "In write_Enum_spec_attr_rw for attribute " << att.get_name() << std::endl;
}

void DevTest::write_attr_hardware(std::vector<long> &att_idx)
{
    switch(wattr_throw)
    {
    case 0:
    default:
        // do nothing
        break;

    case 1:
        TANGO_THROW_EXCEPTION("DevTest_WriteAttrHardware", "DevFailed from write_attr_hardware");
        break;

    case 2:
        if(att_idx.empty() == false)
        {
            Tango::Except::throw_named_exception(get_device_attr()->get_attr_by_ind(att_idx[0]).get_name().c_str(),
                                                 "DevTest_WriteAttrHardware",
                                                 "aaa",
                                                 "DevTest::write_attr_hardware");
        }
        break;

    case 3:
        if(att_idx.size() >= 2)
        {
            Tango::Except::throw_named_exception(
                this, att_idx[2], "DevTest_WriteAttrHardware", "bbb", "DevTest::write_attr_hardware");
        }
        break;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		DevTest::read_xxx
//
// description : 	Read attribute methods
//
//-----------------------------------------------------------------------------

void DevTest::read_Short_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Short_attr" << std::endl;
    attr_short = 12;
    if(Short_attr_except == false)
    {
        att.set_value(&attr_short);
    }
    else
    {
        TANGO_THROW_EXCEPTION("aaa", "This is a test");
    }
}

void DevTest::read_Long_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Long_attr" << std::endl;
    if(Long_attr_except)
    {
        TANGO_THROW_EXCEPTION("Long_attr_except", "Test exception is enabled for this attribute");
    }
    att.set_value(&attr_long);
}

void DevTest::read_Long64_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Long64_attr" << std::endl;
    attr_long64 = 300;
    att.set_value(&attr_long64);
}

void DevTest::read_Double_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Double_attr" << std::endl;
    attr_double = 3.2;
    att.set_value(&attr_double);
}

void DevTest::read_String_attr(Tango::Attribute &att)
{
    static Tango::DevString s = nullptr;

    TANGO_LOG << "[DevTest::read_attr] attribute name String_attr" << std::endl;
    if(s == nullptr)
    {
        s = Tango::string_dup("test_string");
    }

    att.set_value(&s);
}

void DevTest::read_Short_spec_attr(Tango::Attribute &att)
{
    static short attr_short_array[20];

    TANGO_LOG << "[DevTest::read_attr] attribute name Short_spec_attr" << std::endl;
    attr_short_array[0] = 10;
    attr_short_array[1] = 20;
    attr_short_array[2] = 30;
    attr_short_array[3] = 40;

    att.set_value(attr_short_array, 4);
}

void DevTest::read_Long_spec_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Long_spec_attr" << std::endl;
    attr_long_array[0] = 0;
    attr_long_array[1] = 1;
    attr_long_array[2] = 2;
    attr_long_array[3] = 3;
    attr_long_array[4] = 4;
    attr_long_array[5] = 5;
    attr_long_array[6] = 6;
    attr_long_array[7] = 7;
    attr_long_array[8] = 8;
    attr_long_array[9] = 9;

    att.set_value(attr_long_array, 10);
}

void DevTest::read_Double_spec_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Double_spec_attr" << std::endl;
    attr_db_array[0] = 1.11;
    attr_db_array[1] = 2.22;

    att.set_value(attr_db_array, 2);
}

void DevTest::read_String_spec_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name String_spec_attr" << std::endl;
    attr_str_array[0] = (char *) "Hello world";
    attr_str_array[1] = (char *) "Hello universe";

    att.set_value(attr_str_array, 2);
}

void DevTest::read_Short_ima_attr(Tango::Attribute &att)
{
    static short attr_short_array_1[20];

    TANGO_LOG << "[DevTest::read_attr] attribute name Short_ima_attr" << std::endl;
    attr_short_array_1[0] = 40;
    attr_short_array_1[1] = 60;
    attr_short_array_1[2] = 80;
    attr_short_array_1[3] = 100;

    att.set_value(attr_short_array_1, 2, 2);
}

void DevTest::read_Long_ima_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Long_ima_attr" << std::endl;
    attr_long_array[0] = 0;
    attr_long_array[1] = 1;
    attr_long_array[2] = 2;
    attr_long_array[3] = 3;
    attr_long_array[4] = 4;
    attr_long_array[5] = 5;

    att.set_value(attr_long_array, 3, 2);
}

void DevTest::read_Double_ima_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Double_ima_attr" << std::endl;
    attr_db_array[0] = 5.55;
    attr_db_array[1] = 6.66;

    att.set_value(attr_db_array, 2, 1);
}

void DevTest::read_String_ima_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name String_ima_attr" << std::endl;
    attr_str_array[0] = (char *) "Hello milky way";
    attr_str_array[1] = (char *) "Hello moon";

    att.set_value(attr_str_array, 1, 2);
}

void DevTest::read_attr_no_data(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name attr_no_data" << std::endl;
    att.set_quality(Tango::ATTR_VALID);
}

void DevTest::read_attr_wrong_type(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name attr_wrong_type" << std::endl;
    attr_long = 1246;
    att.set_value(&attr_long);
}

void DevTest::read_attr_wrong_size(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name attr_wrong_size" << std::endl;
    att.set_value(&attr_long, 1000, 1000);
}

void DevTest::read_attr_no_alarm(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name attr_no_alarm" << std::endl;
    attr_long = 1246;
    att.check_alarm();
    att.set_value(&attr_long);
}

void DevTest::read_Short_attr_with_w(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Short_attr_with_w" << std::endl;
    attr_short = 44;
    att.set_value(&attr_short);
}

void DevTest::read_Long_attr_with_w(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Long_attr_with_w" << std::endl;
    attr_long = 1246;
    att.set_value(&attr_long);
}

void DevTest::read_Short_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Short_attr_rw" << std::endl;
    att.set_value(&attr_short_rw);
}

void DevTest::read_Long64_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Long64_attr_rw" << std::endl;
    att.set_value(&attr_long64_rw);
}

void DevTest::read_ULong_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name ULong_attr_rw" << std::endl;
    att.set_value(&attr_ulong_rw);
}

void DevTest::read_ULong64_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name ULong64_attr_rw" << std::endl;
    att.set_value(&attr_ulong64_rw);
}

void DevTest::read_State_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name State_attr_rw" << std::endl;
    att.set_value(&attr_state_rw);
}

void DevTest::read_String_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name String_attr_rw" << std::endl;
    att.set_value(&attr_string_rw);
}

void DevTest::read_attr_asyn(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute attr_asyn" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    attr_double = 5.55;
    att.set_value(&attr_double);
    TANGO_LOG << "Leaving reading attr_asyn attribute" << std::endl;
}

void DevTest::read_attr_asyn_to(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute attr_asyn_to" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(4));
    attr_double = 5.55;
    att.set_value(&attr_double);
    TANGO_LOG << "Leaving reading attr_asyn_to attribute" << std::endl;
}

void DevTest::read_attr_asyn_except(TANGO_UNUSED(Tango::Attribute &att))
{
    TANGO_LOG << "[DevTest::read_attr] attribute attr_asyn_except" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    TANGO_LOG << "Leaving reading attr_asyn_except attribute" << std::endl;

    TANGO_THROW_EXCEPTION("aaa", "This is a test");
}

void DevTest::read_PollLong_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name PollLong_attr" << std::endl;
    PollLong_attr_num++;
    if((PollLong_attr_num % 2) == 0)
    {
        Poll_attr_long = 5555;
    }
    else
    {
        Poll_attr_long = 6666;
    }
    att.set_value(&Poll_attr_long);
}

void DevTest::read_PollString_spec_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name PollString_spec_attr" << std::endl;
    PollString_spec_attr_num++;
    if((PollString_spec_attr_num % 4) == 0)
    {
        attr_str_array[0] = (char *) "Hello world";
        attr_str_array[1] = (char *) "Hello universe";
        att.set_value(attr_str_array, 2);
    }
    else if((PollString_spec_attr_num % 4) == 1)
    {
        attr_str_array[0] = (char *) "Hello Grenoble";
        //      		attr_str_array[1] = (char *)"Hello Paris";
        att.set_value(attr_str_array, 1);
    }
    else if((PollString_spec_attr_num % 4) == 2)
    {
        TANGO_THROW_EXCEPTION("aaaa", "bbb");
    }
    else
    {
        TANGO_THROW_EXCEPTION("xxx", "yyy");
    }

    //      	att.set_value(attr_str_array, 2);
}

void DevTest::read_attr_dq_sh(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name attr_dq_sh" << std::endl;
    attr_dq_short = 77;

    att.set_value_date_quality(&attr_dq_short, std::chrono::system_clock::now(), Tango::ATTR_VALID);
}

void DevTest::read_attr_dq_lo(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name attr_dq_lo" << std::endl;
    attr_dq_long = 7777;
    time_t da;
    time(&da);
    att.set_value_date_quality(&attr_dq_long, da, Tango::ATTR_ALARM);
}

void DevTest::read_attr_dq_db(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name attr_dq_db" << std::endl;
    attr_dq_double = 8.888;

    att.set_value_date_quality(&attr_dq_double, std::chrono::system_clock::now(), Tango::ATTR_VALID);
}

void DevTest::read_attr_dq_str(Tango::Attribute &att)
{
    static Tango::DevString s = nullptr;

    TANGO_LOG << "[DevTest::read_attr] attribute name attr_dq_str" << std::endl;
    s = attr_dq_str;
    strcpy(attr_dq_str, "Setting value date and quality");
    time_t da;
    time(&da);
    att.set_value_date_quality(&s, da, Tango::ATTR_ALARM);
}

void DevTest::read_Short_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Short_spec_attr_rw" << std::endl;
    short_spec_attr[0] = 8;
    short_spec_attr[1] = 9;

    att.set_value(short_spec_attr, 2);
}

void DevTest::read_String_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name String_spec_attr_rw" << std::endl;
    string_spec_attr[0] = "Thank's god";
    string_spec_attr[1] = "It's friday";

    att.set_value(const_cast<char **>(string_spec_attr), 2);
}

void DevTest::read_String_empty_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name String_empty_spec_attr_rw" << std::endl;
    att.set_value(const_cast<char **>(string_empty_spec_attr), 0);
}

void DevTest::read_Long_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Long_spec_attr_rw" << std::endl;
    long_spec_attr[0] = 88;
    long_spec_attr[1] = 99;
    long_spec_attr[2] = 111;

    att.set_value(long_spec_attr, 3);
}

void DevTest::read_Short_ima_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Short_ima_attr_rw" << std::endl;
    short_ima_attr[0] = 6;
    short_ima_attr[1] = 7;
    short_ima_attr[2] = 8;
    short_ima_attr[3] = 9;

    att.set_value(short_ima_attr, 2, 2);
}

void DevTest::read_String_ima_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name String_ima_attr_rw" << std::endl;
    string_ima_attr[0] = "Alors la, pour une surprise";
    string_ima_attr[1] = "c'est une surprise";

    att.set_value(const_cast<char **>(string_ima_attr), 2, 1);
}

void DevTest::read_Event_change_tst(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name event_change_tst" << std::endl;
    if(event_change_attr_except == false)
    {
        if(event_throw_out_of_sync == true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
        att.set_value(attr_event, attr_event_size);
    }
    else
    {
        TANGO_THROW_EXCEPTION("bbb", "This is a test");
    }
}

void DevTest::read_Event64_change_tst(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name event64_change_tst" << std::endl;
    if(event_change_attr_except == false)
    {
        att.set_value(attr_event64, attr_event64_size);
    }
    else
    {
        TANGO_THROW_EXCEPTION("bbb64", "This is a test");
    }
}

void DevTest::read_Event_quality_tst(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name event_quality_tst" << std::endl;
    if(event_quality_attr_except == false)
    {
        if(attr_event_qua != Tango::ATTR_INVALID)
        {
            att.set_value(attr_qua_event, 2);
        }
        att.set_quality(attr_event_qua);
    }
    else
    {
        TANGO_THROW_EXCEPTION("ccc", "This is a test");
    }
}

void DevTest::read_Poll_buff(TANGO_UNUSED(Tango::Attribute &att))
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Poll_buff" << std::endl;
}

void DevTest::read_Poll_buffRW(TANGO_UNUSED(Tango::Attribute &att))
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Poll_buffRW" << std::endl;
}

void DevTest::read_Float_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Float_attr" << std::endl;
    attr_float = 4.5;
    att.set_value(&attr_float);
}

void DevTest::read_Boolean_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Boolean_attr" << std::endl;
    attr_boolean = true;
    att.set_value(&attr_boolean);
}

void DevTest::read_UShort_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name UShort_attr" << std::endl;
    attr_ushort = 111;
    att.set_value(&attr_ushort);
}

void DevTest::read_UChar_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name UChar_attr" << std::endl;
    attr_uchar = 88;
    att.set_value(&attr_uchar);
}

void DevTest::read_ULong_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name ULong_attr" << std::endl;
    attr_ulong = 100;
    att.set_value(&attr_ulong);
}

void DevTest::read_ULong64_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name ULong64_attr" << std::endl;
    attr_ulong64 = 200;
    att.set_value(&attr_ulong64);
}

void DevTest::read_Float_spec_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Float_spec_attr" << std::endl;
    attr_spec_float[0] = 4.5;
    attr_spec_float[1] = 8.5;
    attr_spec_float[2] = 16.5;
    att.set_value(attr_spec_float, 3);
}

void DevTest::read_Boolean_spec_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Boolean_spec_attr" << std::endl;
    attr_spec_boolean[0] = true;
    attr_spec_boolean[1] = true;
    attr_spec_boolean[2] = false;
    attr_spec_boolean[3] = true;
    attr_spec_boolean[4] = true;
    att.set_value(attr_spec_boolean, 5);
}

void DevTest::read_UShort_spec_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name UShort_spec_attr" << std::endl;
    attr_spec_ushort[0] = 333;
    attr_spec_ushort[1] = 444;
    att.set_value(attr_spec_ushort, 2);
}

void DevTest::read_UChar_spec_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name UChar_spec_attr" << std::endl;
    attr_spec_uchar[0] = 28;
    attr_spec_uchar[1] = 45;
    attr_spec_uchar[2] = 156;
    attr_spec_uchar[3] = 34;
    attr_spec_uchar[4] = 200;
    attr_spec_uchar[5] = 12;
    att.set_value(attr_spec_uchar, 6);
}

void DevTest::read_Float_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Float_spec_attr_rw" << std::endl;
    attr_spec_float[0] = 5.5;
    attr_spec_float[1] = 11.5;

    att.set_value(attr_spec_float, 2);
}

void DevTest::read_UChar_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name UChar_spec_attr_rw" << std::endl;
    attr_spec_uchar[0] = 22;
    attr_spec_uchar[1] = 44;
    attr_spec_uchar[2] = 66;

    att.set_value(attr_spec_uchar, 3);
}

void DevTest::read_Boolean_ima_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Boolean_ima_attr_rw" << std::endl;
    attr_spec_boolean[0] = true;
    attr_spec_boolean[1] = false;

    att.set_value(attr_spec_boolean, 2, 1);
}

void DevTest::read_UShort_ima_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name UShort_ima_attr_rw" << std::endl;
    attr_spec_ushort[0] = 2;
    attr_spec_ushort[1] = 3;

    att.set_value(attr_spec_ushort, 2, 1);
}

void DevTest::read_slow_actuator(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name slow_actuator" << std::endl;
    slow_actua++;
    att.set_value(&slow_actua);

    if(slow_actua_write.tv_sec != 0)
    {
        auto now = std::chrono::system_clock::now();
        auto delta = now - Tango::make_system_time(slow_actua_write);

        TANGO_LOG << "Delta time = " << Tango::duration_ms(delta) << std::endl;
        if(delta > std::chrono::milliseconds(3000))
        {
            att.set_quality(Tango::ATTR_VALID);
            slow_actua_write.tv_sec = 0;
        }
        else
        {
            att.set_quality(Tango::ATTR_CHANGING);
        }
    }
}

void DevTest::read_fast_actuator(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name fast_actuator" << std::endl;
    att.set_value(&fast_actua);
}

void DevTest::read_Long64_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Long64_spec_attr_rw" << std::endl;
    att.set_value(attr_spec_long64_rw, 3);
}

void DevTest::read_ULong_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name ULong_spec_attr_rw" << std::endl;
    att.set_value(attr_spec_ulong_rw, 3);
}

void DevTest::read_ULong64_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name ULong64_spec_attr_rw" << std::endl;
    att.set_value(attr_spec_ulong64_rw, 3);
}

void DevTest::read_State_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name State_spec_attr_rw" << std::endl;
    att.set_value(attr_spec_state_rw, 2);
}

void DevTest::read_Sub_device_tst(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Sub_device_tst" << std::endl;

    Tango::Util *tg = Tango::Util::instance();

    //
    // sort the devices in the ascending name order
    //

    std::vector<Tango::DeviceImpl *> &dev_list = tg->get_device_list_by_class("DevTest");
    std::vector<Tango::DeviceImpl *> dev_list_sorted = dev_list;

    size_t n = dev_list_sorted.size();
    // the second device on the list is selected to be the sub device, so the list has to comprise of 2 or more elements
    if(n > 1)
    {
        // bubble sort
        do
        {
            size_t i;
            for(i = 0; i < n - 1; i++)
            {
                if(dev_list_sorted[i]->get_name() > dev_list_sorted[i + 1]->get_name())
                {
                    Tango::DeviceImpl *dev_tmp;
                    dev_tmp = dev_list_sorted[i];
                    dev_list_sorted[i] = dev_list_sorted[i + 1];
                    dev_list_sorted[i + 1] = dev_tmp;
                }
            }
            n = i;
        } while(n != 1);

        try
        {
            auto tmp_dev = std::make_shared<Tango::DeviceProxy>(dev_list_sorted[1]->get_name());
            attr_sub_device_tst = true;
        }
        catch(...)
        {
            attr_sub_device_tst = false;
        }
    }
    else
    {
        attr_sub_device_tst = false;
    }

    att.set_value(&attr_sub_device_tst);
}

void DevTest::read_Encoded_attr_rw(Tango::Attribute &att)
{
    //		static int count = 0;
    TANGO_LOG << "[DevTest::read_attr] attribute name Encoded_attr_rw" << std::endl;

    /*		if ((count % 2) == 0)
                    {
                            enc_attr.encoded_format = Tango::string_dup("First string");
                            enc_attr.encoded_data[0] = 11;
                    }
                    else
                    {
                            enc_attr.encoded_format = Tango::string_dup("Second string");
                            enc_attr.encoded_data[0] = 22;
                    }*/

    att.set_value(&enc_attr);
    //		count++;

    /*		enc_attr_ptr = new Tango::DevEncoded;
                    enc_attr_ptr->encoded_format = Tango::string_dup("Which format?");
                    enc_attr_ptr->encoded_data.length(4);
                    enc_attr_ptr->encoded_data[0] = (unsigned char)97;
                    enc_attr_ptr->encoded_data[1] = (unsigned char)98;
                    enc_attr_ptr->encoded_data[2] = (unsigned char)99;
                    enc_attr_ptr->encoded_data[3] = (unsigned char)100;

            att.set_value(enc_attr_ptr,1,0,true);*/

    /*		char *str_ptr = new char[80];
                    strcpy(str_ptr,"Hola");

                    unsigned char *data_ptr = new unsigned char [4];
                    data_ptr[0] = (unsigned char)79;
                    data_ptr[1] = (unsigned char)80;
                    data_ptr[2] = (unsigned char)81;
                    data_ptr[3] = (unsigned char)82;

                    att.set_value(&str_ptr,data_ptr,4,true);*/

    /*		att.set_value(&enc_format,&(enc_data[0]),2);	*/
}

void DevTest::read_Encoded_attr_image(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Encoded_attr_image" << std::endl;

    // Build a dummy image
    unsigned char *imageData = (unsigned char *) malloc(256 * 256);
    for(int i = 0; i < 256; i++)
    {
        for(int j = 0; j < 256; j++)
        {
            imageData[i + j * 256] = i;
        }
    }

    // Encode a JPEG image
    jpeg.encode_jpeg_gray8(imageData, 256, 256, 50.0);
    att.set_value(&jpeg);
    free(imageData);
}

void DevTest::read_Slow_attr(Tango::Attribute &att)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    att.set_value(&attr_slow);
}

void DevTest::read_Def_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name DefAttr" << std::endl;
    att.set_value(&att_conf);
}

void DevTest::read_DefUser_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name DefUserAttr" << std::endl;
    att.set_value(&att_conf);
}

void DevTest::read_DefClass_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name DefClassAttr" << std::endl;
    att.set_value(&att_conf);
}

void DevTest::read_DefClassUser_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name DefClassUserAttr" << std::endl;
    att.set_value(&att_conf);
}

void DevTest::read_Enum_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name EnumAttr" << std::endl;
    cp = SOUTH;
    att.set_value(&cp);
}

void DevTest::read_Enum_spec_attr_rw(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name Enum_spec_attr_rw" << std::endl;
    cp_array[0] = SOUTH;
    cp_array[1] = NORTH;
    cp_array[2] = WEST;
    att.set_value(cp_array, 3);
}

void DevTest::read_DynEnum_attr(Tango::Attribute &att)
{
    TANGO_LOG << "[DevTest::read_attr] attribute name DynEnum_attr" << std::endl;
    att.set_value(&enum_value);
}

void DevTest::read_ReynaldPoll_attr(TANGO_UNUSED(Tango::Attribute &att))
{
    TANGO_LOG << "[DevTest::read_attr] attribute name ReynaldPoll_attr" << std::endl;

    Reynald_ctr++;
    if((Reynald_ctr % 3) == 0)
    {
        int poll_period = get_attribute_poll_period("Short_attr");
        if(poll_period == 250)
        {
            TANGO_LOG << "ReynaldPollAttr: Setting period to 500" << std::endl;
            poll_attribute("Short_attr", 500);
            poll_attribute("ReynaldPollAttr", 500);
        }
        else
        {
            TANGO_LOG << "ReynaldPollAttr: Setting period to 250" << std::endl;
            poll_attribute("Short_attr", 250);
            poll_attribute("ReynaldPollAttr", 250);
        }
    }
}

//***********************************************************************************************
//
//				Pipe related methods
//
//***********************************************************************************************

// RPipe

bool DevTest::is_RPipe_allowed(Tango::PipeReqType)
{
    if(get_state() == Tango::ON)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void DevTest::read_RPipe(Tango::Pipe &pipe)
{
    switch(rpipe_type)
    {
    case 0:
    {
        pipe.set_root_blob_name("BlobCase0");

        std::vector<std::string> de_names;
        de_names.push_back(std::string("FirstDE"));
        de_names.push_back(std::string("SecondDE"));
        de_names.push_back(std::string("ThirdDE"));
        de_names.push_back(std::string("ForthDE"));

        pipe.set_data_elt_names(de_names);

        dl = 666;
        v_db.clear();
        v_db.push_back(1.11);
        v_db.push_back(2.22);

        unsigned short *array = new unsigned short[100];
        for(unsigned short i = 0; i < 100; i++)
        {
            array[i] = i;
        }
        Tango::DevVarUShortArray *dvush = create_DevVarUShortArray(array, 100);

        dvsa.length(2);
        dvsa[0] = Tango::ON;
        dvsa[1] = Tango::OFF;

        try
        {
            pipe << dl << v_db << dvush << dvsa;
            TANGO_LOG << "Data type 0 inserted in pipe" << std::endl;
        }
        catch(Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            throw e;
        }
    }
    break;

    case 1:
    {
        pipe.set_root_blob_name("BlobCase1");

        pipe.set_data_elt_nb(5);

        pipe_str.name = "stringDE";
        pipe_str.value = "Hello";

        pipe_devstr.name = "DevStringDE";
        pipe_devstr.value = const_cast<Tango::DevString>("Hola");

        pipe_enc.name = "DevEncodedDE";
        pipe_enc.value.encoded_format = "Format";
        pipe_enc.value.encoded_data.length(2);
        pipe_enc.value.encoded_data[0] = 0;
        pipe_enc.value.encoded_data[1] = 1;

        pipe_v_str.name = "VectorStringDE";
        pipe_v_str.value.clear();
        pipe_v_str.value.push_back(std::string("Bonjour"));
        pipe_v_str.value.push_back(std::string("le"));
        pipe_v_str.value.push_back(std::string("monde"));

        pipe_dvsa.name = "DevVarStringArrayDE";
        pipe_dvsa.value.length(1);
        (pipe_dvsa.value)[0] = "Why not?";

        try
        {
            pipe << pipe_str << pipe_devstr << pipe_enc << pipe_v_str << pipe_dvsa;
            TANGO_LOG << "Data type 1 inserted in pipe" << std::endl;
        }
        catch(Tango::DevFailed &e)
        {
            Tango::Except::print_exception(e);
            throw e;
        }
    }
    break;

    case 2:
    {
        pipe.set_root_blob_name("BlobCase2");

        std::vector<std::string> de_names;
        de_names.push_back(std::string("FirstDE"));
        de_names.push_back(std::string("SecondDE"));
        pipe.set_data_elt_names(de_names);

        dl = 999;
        v_db.clear();
        v_db.push_back(3.33);
        v_db.push_back(4.44);
        v_db.push_back(5.55);

        pipe["SecondDE"] << v_db;
        pipe["FirstDE"] << dl;

        TANGO_LOG << "Data type 2 inserted in pipe" << std::endl;
    }
    break;

    case 3:
    {
        pipe.set_root_blob_name("BlobCase3");

        std::vector<std::string> de_inner_inner_names;
        de_inner_inner_names.push_back(std::string("InnerInnerFirstDE"));
        de_inner_inner_names.push_back("InnerInnerSecondDE");
        inner_inner_blob.set_data_elt_names(de_inner_inner_names);
        inner_inner_blob.set_name("InnerInner");

        dl = 111;
        v_db.clear();
        v_db.push_back(3.33);

        inner_inner_blob["InnerInnerSecondDE"] << v_db;
        inner_inner_blob["InnerInnerFirstDE"] << dl;

        std::vector<std::string> de_inner_names;
        de_inner_names.push_back("InnerFirstDE");
        de_inner_names.push_back("InnerSecondDE");
        de_inner_names.push_back("InnerThirdDE");
        inner_blob.set_data_elt_names(de_inner_names);
        inner_blob.set_name("Inner");

        inner_str = "Grenoble";
        inner_bool = true;

        inner_blob << inner_str << inner_inner_blob << inner_bool;

        std::vector<std::string> de_names;
        de_names.push_back("1DE");
        de_names.push_back("2DE");
        pipe.set_data_elt_names(de_names);

        v_dl.clear();
        v_dl.push_back(3);
        v_dl.push_back(4);
        v_dl.push_back(5);
        v_dl.push_back(6);

        pipe << inner_blob << v_dl;

        TANGO_LOG << "Data type 3 inserted in pipe" << std::endl;
    }
    break;

    case 4:
    {
        pipe.set_root_blob_name("BlobCase4");
        std::vector<std::string> de_names;
        de_names.push_back("1DE");
        de_names.push_back("2DE");
        pipe.set_data_elt_names(de_names);

        pipe << dl;

        TANGO_LOG << "Data type 4 inserted in pipe" << std::endl;
    }
    break;

    case 5:
    {
        pipe.set_root_blob_name("BlobCase5");
        std::vector<std::string> de_names;
        de_names.push_back("1DE");
        pipe.set_data_elt_names(de_names);

        TANGO_LOG << "Data type 5 inserted in pipe" << std::endl;
    }
    break;

    case 6:
    {
        pipe.set_root_blob_name("BlobCase6");
        std::vector<std::string> de_names;
        de_names.push_back("1DE");
        de_names.push_back("2DE");
        de_names.push_back("1de");
        pipe.set_data_elt_names(de_names);
    }
    break;

    case 7:
    {
        pipe.set_root_blob_name("BlobCase7");
        pipe << dl;
    }
    break;

    case 8:
    {
        pipe.set_root_blob_name("BlobCase8");

        std::vector<std::string> de_names;
        de_names.push_back("1DE");
        de_names.push_back("2DE");
        de_names.push_back("3de");
        pipe.set_data_elt_names(de_names);

        pipe << dl;
        pipe["2DE"] << v_dl;
    }
    break;

    case 9:
    {
        std::vector<std::string> de_names;
        de_names.push_back("Another_1DE");
        de_names.push_back("Another_2DE");

        v_dl.clear();
        v_dl.push_back(2);
        std::string str("Barcelona");

        pipe.set_root_blob_name("BlobCase9");
        pipe.set_data_elt_names(de_names);
        pipe << v_dl << str;

        Tango::DevicePipeBlob &bl = pipe.get_blob();
        this->push_pipe_event("RPipe", &bl, true);
    }
    break;

    case 10:
    {
        pipe.set_root_blob_name("BlobCase10");

        std::vector<std::string> de_names;
        de_names.push_back("1DE");
        pipe.set_data_elt_names(de_names);

        v_dl.clear();
        v_dl.push_back(22);
        std::string str("Barcelona");

        pipe << v_dl << str;
    }
    break;

    case 11:
    {
        pipe.set_root_blob_name("BlobCase11");

        std::vector<std::string> de_names;
        de_names.push_back("1DE");
        de_names.push_back("2DE");
        pipe.set_data_elt_names(de_names);

        Tango::DevULong64 dul64 = 123456;
        v_bool.clear();
        v_bool.push_back(true);
        v_bool.push_back(true);
        v_bool.push_back(false);

        pipe << dul64 << v_bool;
    }
    break;

    case 12:
    {
        pipe.set_root_blob_name("BlobCase12");

        std::vector<std::string> de_names;
        de_names.push_back("1DE_Array");
        de_names.push_back("2DE_Array");
        pipe.set_data_elt_names(de_names);

        v_bool.clear();
        v_bool.push_back(false);
        v_dl.clear();
        v_dl.push_back(9);

        pipe << v_dl << v_bool;
    }
    break;
    }
}

// RWPipe

bool DevTest::is_RWPipe_allowed(Tango::PipeReqType)
{
    return true;
}

void DevTest::read_RWPipe(Tango::Pipe &pipe)
{
    pipe_mutex.lock();
    pipe.set_user_pipe_mutex(&pipe_mutex);

    pipe.set_root_blob_name("RWPipeBlob");

    std::vector<std::string> de_names;
    de_names.push_back("RW_1DE");
    de_names.push_back("RW_2DE");
    pipe.set_data_elt_names(de_names);

    dl = 666;
    v_db.clear();
    v_db.push_back(1.11);
    v_db.push_back(2.22);

    try
    {
        pipe << dl << v_db;
    }
    catch(Tango::DevFailed &e)
    {
        Tango::Except::print_exception(e);
        throw e;
    }
}

void DevTest::write_RWPipe(Tango::WPipe &w_pipe)
{
    std::string str;
    std::vector<float> v_fl;

    std::vector<std::string> de_names = w_pipe.get_data_elt_names();
    for(size_t loop = 0; loop < de_names.size(); loop++)
    {
        TANGO_LOG << "RWPipe: Received data element name = " << de_names[loop] << std::endl;
    }

    w_pipe >> str >> v_fl;
}

// RPipeDE

bool DevTest::is_RPipeDE_allowed(Tango::PipeReqType)
{
    if(get_state() == Tango::ON)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void DevTest::read_RPipeDE(Tango::Pipe &pipe)
{
    pipe.set_root_blob_name("BlobDE");

    std::vector<std::string> de_names;
    de_names.push_back(std::string("FirstDE"));
    de_names.push_back(std::string("SecondDE"));
    de_names.push_back(std::string("ThirdDE"));
    de_names.push_back(std::string("ForthDE"));

    pipe.set_data_elt_names(de_names);

    dl = 666;
    v_db.clear();
    v_db.push_back(1.11);
    v_db.push_back(2.22);

    unsigned short *array = new unsigned short[100];
    for(unsigned short i = 0; i < 100; i++)
    {
        array[i] = i;
    }
    Tango::DevVarUShortArray *dvush = create_DevVarUShortArray(array, 100);

    dvsa.length(2);
    dvsa[0] = Tango::ON;
    dvsa[1] = Tango::OFF;

    try
    {
        pipe << dl << v_db << dvush << dvsa;
        TANGO_LOG << "Data inserted in pipe (using DataElement classes)" << std::endl;
    }
    catch(Tango::DevFailed &e)
    {
        Tango::Except::print_exception(e);
        throw e;
    }
}

void DevTest::cmd_push_pipe_event(Tango::DevShort in)
{
    if(in == 0)
    {
        Tango::DevicePipeBlob dpb("PipeEventCase0");

        std::vector<std::string> de_inner_inner_names;
        de_inner_inner_names.push_back("InnerInnerFirstDE");
        de_inner_inner_names.push_back("InneraaaaaaaInnerSecondDE");
        inner_inner_blob.set_data_elt_names(de_inner_inner_names);
        inner_inner_blob.set_name("InnerInner");

        dl = 111;
        v_db.clear();
        v_db.push_back(3.33);
        v_db.push_back(3.33);

        inner_inner_blob["InneraaaaaaaInnerSecondDE"] << v_db;
        inner_inner_blob["InnerInnerFirstDE"] << dl;

        std::vector<std::string> de_inner_names;
        de_inner_names.push_back("InnerFirstDE");
        de_inner_names.push_back("InnerSecondDE");
        de_inner_names.push_back("InnerThirdDE");
        inner_blob.set_data_elt_names(de_inner_names);
        inner_blob.set_name("Inner");

        inner_str = "Grenoble";
        inner_bool = true;

        inner_blob << inner_str << inner_inner_blob << inner_bool;

        std::vector<std::string> de_names;
        de_names.push_back("1DE");
        de_names.push_back("2DE");
        dpb.set_data_elt_names(de_names);

        v_dl.clear();
        v_dl.push_back(3);
        v_dl.push_back(4);
        v_dl.push_back(5);
        v_dl.push_back(6);

        dpb << inner_blob << v_dl;
        this->push_pipe_event("RWPipe", &dpb);
    }
    else if(in == 1)
    {
        Tango::DevicePipeBlob dpb("PipeEventCase1");
        std::vector<std::string> de_names;
        de_names.push_back("Another_1DE");
        de_names.push_back("Another_2DE");
        dpb.set_data_elt_names(de_names);

        v_dl.clear();
        v_dl.push_back(2);
        std::string str("Barcelona");

        dpb << v_dl << str;

        this->push_pipe_event("RWPipe", &dpb);
    }
    else if(in == 2)
    {
        Tango::DevicePipeBlob dpb("PipeEventCase2");
        std::vector<std::string> de_names;
        de_names.push_back("Qwerty_1DE");
        de_names.push_back("Azerty_2DE");
        dpb.set_data_elt_names(de_names);

        std::string str("Barcelona");

        dpb << str << v_dl;

        this->push_pipe_event("RWPipe", &dpb, std::chrono::system_clock::time_point(std::chrono::seconds(10)));
    }
    else if(in == 3)
    {
        Tango::DevErrorList del;
        del.length(1);
        del[0].reason = Tango::string_dup("aaa");
        del[0].desc = Tango::string_dup("bbb");
        del[0].origin = Tango::string_dup("ccc");
        Tango::DevFailed df(del);
        this->push_pipe_event("RWPipe", &df);
    }
    else if(in == 4)
    {
        Tango::DevicePipeBlob dpb("PipeEventCase4");
        std::vector<std::string> de_names;
        de_names.push_back("Lunes");
        de_names.push_back("Martes");
        dpb.set_data_elt_names(de_names);

        std::string str("Girona");

        v_dl.clear();
        for(int loop = 0; loop < 3000; loop++)
        {
            v_dl.push_back(loop);
        }

        dpb << str << v_dl;

        this->push_pipe_event("RWPipe", &dpb);
    }
}

// ConfPipe

bool DevTest::is_ConfPipe_allowed(Tango::PipeReqType)
{
    if(get_state() == Tango::ON)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void DevTest::read_ConfPipe(TANGO_UNUSED(Tango::Pipe &pipe)) { }

void DevTest::cmd_push_state_status_event()
{
    set_change_event("State", true, false);
    set_change_event("Status", true, false);

    this->push_change_event("state");
    this->push_change_event("status");

    set_change_event("State", false, false);
    set_change_event("Status", false, false);
}

// NOLINTEND(*)
