// NOLINTBEGIN(*)

#ifdef WIN32
  #include <process.h>
#endif

#include "old_common.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if(argc != 4)
    {
        TEST_LOG << "usage: " << argv[0] << " <device> <full ds name> <idlver>" << endl;
        exit(-1);
    }

    string device_name = argv[1];
    string ds_name = argv[2];
    string admin_device("dserver/");
    admin_device = admin_device + ds_name;
    int idlver = parse_as<int>(argv[3]);

    try
    {
        device = new DeviceProxy(device_name);
    }
    catch(CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    TEST_LOG << '\n' << "new DeviceProxy(" << device->name() << ") returned" << '\n' << endl;

    int elapsed;
    try
    {
        // Test get_timeout

        int to;
        to = device->get_timeout_millis();

        assert(to == 3000);
        TEST_LOG << "   Get timeout --> OK" << endl;

        // Test set_timeout

        int new_to;
        device->set_timeout_millis(2000);
        new_to = device->get_timeout_millis();

        assert(new_to == 2000);

        TEST_LOG << "   Set timeout --> OK" << endl;

        device->set_timeout_millis(3000);

        // Test ping

        elapsed = device->ping();

        TEST_LOG << "   Ping ( " << elapsed << " us ) --> OK" << endl;

        // Test state

        DevState sta;
        sta = device->state();

        assert(sta == Tango::ON);
        TEST_LOG << "   State --> OK" << endl;

        // Test status

        string str;
        str = device->status();

        assert(str == "The device is in ON state.");
        TEST_LOG << "   Status --> OK" << endl;

        // Test Tango lib version

        int tg_version;
        tg_version = device->get_tango_lib_version();

        assert(tg_version >= 810);
        TEST_LOG << "   Tango lib version --> " << tg_version << endl;

        // Test adm_name

        string str_adm;
        str_adm = device->adm_name();

        transform(str_adm.begin(), str_adm.end(), str_adm.begin(), ::tolower);
        transform(admin_device.begin(), admin_device.end(), admin_device.begin(), ::tolower);

        assert(str_adm == admin_device);
        TEST_LOG << "   Adm_name --> OK" << endl;

        // Test description

        string desc;
        desc = device->description();

        assert(desc == "A TANGO device");
        TEST_LOG << "   Description --> OK" << endl;

        // Test name

        string name;
        name = device->name();

        assert(name == device_name);
        TEST_LOG << "   Name --> OK" << endl;

        // Test blackbox

        vector<string> *ptr;
        ptr = device->black_box(3);

        assert(ptr->size() == 3);

        string tmp = (*ptr)[0];
        string::size_type pos, end;
        pos = tmp.find('A');
        end = tmp.find("from");
        string ans = tmp.substr(pos, end - pos);
        assert(ans == "Attribute name requested ");

        tmp = (*ptr)[1];
        end = tmp.find("from");
        ans = tmp.substr(pos, end - pos);
        assert(ans == "Attribute description requested ");

        tmp = (*ptr)[2];
        end = tmp.find("from");
        ans = tmp.substr(pos, end - pos);
        assert(ans == "Attribute adm_name requested ");

        TEST_LOG << "   Black box --> OK" << endl;
        delete ptr;

        // Test info

        DeviceInfo inf;
        inf = device->info();
        assert(inf.dev_class == "DevTest");

        transform(inf.server_id.begin(), inf.server_id.end(), inf.server_id.begin(), ::tolower);
        transform(ds_name.begin(), ds_name.end(), ds_name.begin(), ::tolower);

        assert(inf.server_id == ds_name);
        assert(inf.doc_url == "Doc URL = http://www.tango-controls.org");
        assert(inf.dev_type == "TestDevice");

        TEST_LOG << "   Info --> OK" << endl;

        const int vi_size = static_cast<int>(inf.version_info.size());
        TEST_LOG << "version_info.size --> " << vi_size << endl;

        for(const auto &[key, value] : inf.version_info)
        {
            TEST_LOG << "\t" << key << " : " << value << endl;
        }

        // Test command_query

        DevCommandInfo cmd_info;
        cmd_info = device->command_query("IODoubleArray");

        assert(cmd_info.cmd_name == "IODoubleArray");
        assert(cmd_info.in_type == DEVVAR_DOUBLEARRAY);
        assert(cmd_info.out_type == DEVVAR_DOUBLEARRAY);
        assert(cmd_info.in_type_desc == "Array of double");
        assert(cmd_info.out_type_desc == "This array * 2");

        TEST_LOG << "   Command_query --> OK" << endl;

        // Test command_list_query and get_command_list

        CommandInfoList *cmd_list;
        cmd_list = device->command_list_query();
        TEST_LOG << "cmd list size = " << cmd_list->size() << endl;

        vector<string> *cmd_name_list;
        cmd_name_list = device->get_command_list();

        TEST_LOG << "cmd_name_list size = " << cmd_name_list->size() << endl;

        assert(cmd_name_list->size() == cmd_list->size());

        TEST_LOG << "   Command list --> OK" << endl;

        //        assert (cmd_list->size() == 88 );
        //        assert ((*cmd_list)[0].cmd_name == "FileDb" );
        //        assert ((*cmd_list)[87].cmd_name == "Status");

        //        TEST_LOG << "   Command_list_query --> OK" << endl;
        delete cmd_list;
        delete cmd_name_list;

        // Test get_attribute_list

        vector<string> *att_list;
        att_list = device->get_attribute_list();

        TEST_LOG << "att_list size = " << att_list->size() << endl;
        //        assert ( att_list->size() == 77 );
        //        assert ( (*att_list)[0] == "Short_attr");
        //        assert ( (*att_list)[1] == "Long_attr");
        //        assert ( (*att_list)[21] == "String_attr_w");

        //        TEST_LOG << "   Get attribute list --> OK" << endl;
        delete att_list;

        // Test attribute query

        DeviceAttributeConfig attr_conf;
        attr_conf = device->attribute_query("Short_attr");

        assert(attr_conf.name == "Short_attr");
        assert(attr_conf.data_format == SCALAR);
        assert(attr_conf.data_type == DEV_SHORT);
        assert(attr_conf.description == "No description");
        assert(attr_conf.max_dim_x == 1);

        TEST_LOG << "   Attribute config --> OK" << endl;

        // Test get_attribute_config

        AttributeInfoList *attr_conf_ptr;
        vector<string> li;
        li.push_back("Long_attr");
        li.push_back("Double_attr");

        attr_conf_ptr = device->get_attribute_config(li);

        assert(attr_conf_ptr->size() == 2);

        assert((*attr_conf_ptr)[0].name == "Long_attr");
        assert((*attr_conf_ptr)[0].data_format == SCALAR);
        assert((*attr_conf_ptr)[0].data_type == DEV_LONG);

        assert((*attr_conf_ptr)[1].name == "Double_attr");
        assert((*attr_conf_ptr)[1].data_format == SCALAR);
        assert((*attr_conf_ptr)[1].data_type == DEV_DOUBLE);

        TEST_LOG << "   Get attribute config --> OK" << endl;
        delete attr_conf_ptr;

        //  Test get_command_config

        CommandInfoList *cmd_conf_ptr;
        li.clear();
        li.push_back("state");
        li.push_back("status");

        cmd_conf_ptr = device->get_command_config(li);

        assert(cmd_conf_ptr->size() == 2);

        assert((*cmd_conf_ptr)[0].cmd_name == "State");
        assert((*cmd_conf_ptr)[0].in_type == DEV_VOID);
        assert((*cmd_conf_ptr)[0].out_type == DEV_STATE);

        assert((*cmd_conf_ptr)[1].cmd_name == "Status");
        assert((*cmd_conf_ptr)[1].in_type == DEV_VOID);
        assert((*cmd_conf_ptr)[1].out_type == DEV_STRING);
        delete cmd_conf_ptr;

        TEST_LOG << "   Get command config --> OK" << endl;

        // test attribute_list_query

        //        AttributeInfoList *attr_confs;
        //        attr_confs = device->attribute_list_query();

        //        assert ( attr_confs->size() == 77 );
        //        assert ( (*attr_confs)[0].name == "Short_attr");
        //        assert ( (*attr_confs)[0].data_format == SCALAR);
        //        assert ( (*attr_confs)[0].data_type == DEV_SHORT);

        //        assert ( (*attr_confs)[1].name == "Long_attr");
        //        assert ( (*attr_confs)[1].data_type == DEV_LONG);
        //        assert ( (*attr_confs)[1].data_format == SCALAR);

        //        assert ( (*attr_confs)[21].name == "String_attr_w");
        //        assert ( (*attr_confs)[21].data_type == DEV_STRING);
        //        assert ( (*attr_confs)[21].data_format == SCALAR);

        //        TEST_LOG << "   Attribute list query --> OK "  << endl;
        //        delete attr_confs;

        // Test set_attribute_config

        AttributeInfoList v_conf;
        AttributeInfo co = device->attribute_query("Short_attr");

#ifdef WIN32
        int pid = _getpid();
#else
        pid_t pid = getpid();
#endif

        stringstream st;

        string s;
        st << pid;
        st >> s;

        co.format = s;
        v_conf.push_back(co);

        device->set_attribute_config(v_conf);

        DeviceAttributeConfig res = device->attribute_query("Short_attr");
        assert(res.format == s);

        TEST_LOG << "   Set attribute config --> OK" << endl;

        // Test device version

        int vers = device->get_idl_version();
        assert(vers == idlver);

        TEST_LOG << "   Get IDL version --> OK" << endl;

        // Test source

        Tango::DevSource so = device->get_source();
        assert(so == Tango::CACHE_DEV);

        device->set_source(Tango::DEV);
        assert(device->get_source() == Tango::DEV);

        device->set_source(Tango::CACHE_DEV);

        TEST_LOG << "   Source parameter --> OK" << endl;

        // Test get property list

        vector<string> props;
        device->get_property_list("*", props);

        TEST_LOG << "NB prop = " << props.size() << endl;
        for(unsigned long l = 0; l < props.size(); l++)
        {
            TEST_LOG << "prop = " << props[l] << endl;
        }
        // TODO conf_devtest defines only 3 properties
        assert(props.size() == 3);
        assert(props[0] == "cmd_min_poll_period");
        assert(props[1] == "min_poll_period");
        assert(props[2] == "tst_property");
        //        assert (props[3] == "__SubDevices");

        TEST_LOG << "   Get property list --> OK" << endl;
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(-1);
    }

    delete device;

    return 0;
}

// NOLINTEND(*)
