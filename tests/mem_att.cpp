// NOLINTBEGIN(*)

#include "old_common.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if((argc == 1) || (argc > 3))
    {
        TEST_LOG << "usage: %s device" << endl;
        exit(-1);
    }

    string device_name = argv[1];

    try
    {
        device = new DeviceProxy(device_name);
    }
    catch(CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    TEST_LOG << endl << "new DeviceProxy(" << device->name() << ") returned" << endl << endl;

    //**************************************************************************
    //
    //        Test for a short attribute
    //
    //**************************************************************************

    try
    {
        string short_att_name("Short_attr_w");
        string string_att_name("String_attr_w");
        string bool_att_name("Boolean_attr_w");

        // Write  these attributes

        short sh = 345;
        string str("Do you want to dance?");
        bool bo = true;

        vector<DeviceAttribute> v_da;
        v_da.push_back(DeviceAttribute(short_att_name, sh));
        v_da.push_back(DeviceAttribute(string_att_name, str));
        v_da.push_back(DeviceAttribute(bool_att_name, bo));

        device->write_attributes(v_da);

        // Restart the server

        string adm_name = device->adm_name();
        DeviceProxy adm_dev(adm_name);

#ifdef VALGRIND
        adm_dev.set_timeout_millis(15000);
#endif
        adm_dev.command_inout("RestartServer");

        std::this_thread::sleep_for(std::chrono::milliseconds(3500));

        delete device;
        device = new DeviceProxy(device_name);

        // Read attributes value

        vector<string> ra;
        ra.push_back(short_att_name);
        ra.push_back(string_att_name);
        ra.push_back(bool_att_name);

        vector<DeviceAttribute> *r_att;

        r_att = device->read_attributes(ra);

        short read_sh;
        string read_str;
        bool read_bo;

        (*r_att)[0] >> read_sh;
        (*r_att)[1] >> read_str;
        (*r_att)[2] >> read_bo;
        delete r_att;
        r_att = nullptr;

        TEST_LOG << "Read value for Short_attr_w = " << read_sh << endl;
        TEST_LOG << "Read value for String_attr_w = " << read_str << endl;
        TEST_LOG << "Read value for Boolean_attr_w = " << read_bo << endl;

        assert(read_sh == sh);
        assert(read_str == str);
        assert(read_bo == bo);

        TEST_LOG << "   Memorized attributes --> OK" << endl;

        // Reset the boolean attribute which is part of the device server
        // output message taken into account in the automatic sequence

        DeviceAttribute da_bool("Boolean_attr_w", false);

        device->write_attribute(da_bool);

        // Try to change min_value then max_value with non-coherent value
        // The memorized value is 345

        AttributeInfoListEx *att_conf = nullptr;

        vector<string> att_conf_list;
        att_conf_list.push_back(short_att_name);

        att_conf = device->get_attribute_config_ex(att_conf_list);

        string old_min_value = (*att_conf)[0].min_value;
        (*att_conf)[0].min_value = "500";
        bool except = false;

        try
        {
            device->set_attribute_config(*att_conf);
        }
        catch(Tango::DevFailed &)
        {
            except = true;
            (*att_conf)[0].min_value = old_min_value;
        }

        assert(except == true);
        except = false;

        AttributeInfoListEx *att_conf2 = nullptr;
        att_conf2 = device->get_attribute_config_ex(att_conf_list);

        assert((*att_conf)[0].min_value == (*att_conf2)[0].min_value);
        assert((*att_conf)[0].max_value == (*att_conf2)[0].max_value);

        string old_max_value = (*att_conf2)[0].max_value;
        (*att_conf2)[0].max_value = "200";

        try
        {
            device->set_attribute_config(*att_conf2);
        }
        catch(Tango::DevFailed &)
        {
            except = true;
            (*att_conf2)[0].max_value = old_max_value;
        }

        assert(except == true);
        except = false;

        AttributeInfoListEx *att_conf3 = nullptr;
        att_conf3 = device->get_attribute_config_ex(att_conf_list);

        assert((*att_conf2)[0].min_value == (*att_conf3)[0].min_value);
        assert((*att_conf2)[0].max_value == (*att_conf3)[0].max_value);

        // Set a coherent min_value,max_value

        (*att_conf2)[0].max_value = "400";
        (*att_conf2)[0].min_value = "200";

        try
        {
            device->set_attribute_config(*att_conf2);
        }
        catch(Tango::DevFailed &)
        {
            except = true;
        }

        assert(except == false);

        // Read conf

        AttributeInfoListEx *att_conf4 = nullptr;
        att_conf4 = device->get_attribute_config_ex(att_conf_list);

        assert((*att_conf4)[0].min_value == "200");
        assert((*att_conf4)[0].max_value == "400");

        // Reset min_value,max_value

        (*att_conf2)[0].max_value = "NaN";
        (*att_conf2)[0].min_value = "NaN";

        try
        {
            device->set_attribute_config(*att_conf2);
        }
        catch(Tango::DevFailed &)
        {
            except = true;
        }

        assert(except == false);

        TEST_LOG << "   Setting min_value/max_value for memorized attributes --> OK" << endl;

        delete att_conf;
        delete att_conf2;
        delete att_conf3;
        delete att_conf4;
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    delete device;
    return 0;
}

// NOLINTEND(*)
