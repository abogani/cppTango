#include "common.h"

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

    try
    {
        //
        // Get attribute config
        //

        string att("attr_dq_db");
        AttributeInfo sta_ai = device->get_attribute_config(att);
        TEST_LOG << sta_ai << endl;

        //
        // Get attr value
        //

        DeviceAttribute da = device->read_attribute(att);
        TEST_LOG << da << endl;
        double att_value;
        da >> att_value;

        //
        // Change format
        //

        AttributeInfoList new_ai;
        new_ai.push_back(sta_ai);
        new_ai[0].format = "scientific;uppercase;setprecision(2)";

        device->set_attribute_config(new_ai);

        //
        // Print formatted output
        //

        AttributeInfo sta_ai_2 = device->get_attribute_config(att);
        TEST_LOG << sta_ai_2 << endl;

        TEST_LOG << "Formatted value = " << Tango::AttrManip(sta_ai_2.format) << att_value << endl;

        //
        // Set another format
        //

        new_ai[0].format = "fixed;setprecision(2)";

        device->set_attribute_config(new_ai);

        //
        // Print formatted output
        //

        sta_ai_2 = device->get_attribute_config(att);
        TEST_LOG << sta_ai_2 << endl;

        TEST_LOG << "Formatted value = " << Tango::AttrManip(sta_ai_2.format) << att_value << endl;

        //
        // Reset attribute config
        //

        new_ai[0].format = "";

        device->set_attribute_config(new_ai);
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(-1);
    }

    return 0;
}
