#include "common.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if ((argc == 1) || (argc > 3))
    {
        TEST_LOG << "usage: %s device" << endl;
        exit(-1);
    }

    string device_name = argv[1];

    try
    {
        device = new DeviceProxy(device_name);
    }
    catch (CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    TEST_LOG << endl << "new DeviceProxy(" << device->name() << ") returned" << endl << endl;

    try
    {

//
// Get attribute number
//

        vector<string> *att_list;
        att_list = device->get_attribute_list();
        long nb_att = att_list->size();

        TEST_LOG << "Device attribute number before add = " << nb_att << endl;

//
// Add one attribute to the device
//

        Tango::ConstDevString ds = "Added_short_attr";
        Tango::DeviceData din;
        din << ds;
        device->command_inout("IOAddAttribute",din);

//
// Get new attribute number
//

        vector<string> *att_list_add;
        att_list_add = device->get_attribute_list();
        long nb_att_add = att_list_add->size();

        TEST_LOG << "Device attribute number after add = " << nb_att_add << endl;
        assert (nb_att_add == (nb_att + 1));

//
// Redo it once, this should not change att number (attribute already added)
//

        device->command_inout("IOAddAttribute",din);

        vector<string> *att_list_add2;
        att_list_add2 = device->get_attribute_list();
        long nb_att_add2 = att_list_add2->size();

        TEST_LOG << "Device attribute number after second add = " << nb_att_add2 << endl;
        assert (nb_att_add2 == (nb_att + 1));

        TEST_LOG << "   Adding attribute --> OK" << endl;

//
// Remove the attribute
//

        device->command_inout("IORemoveAttribute",din);

        vector<string> *att_list_rem;
        att_list_rem = device->get_attribute_list();
        long nb_att_rem = att_list_rem->size();

        TEST_LOG << "Device attribute number after removing add = " << nb_att_rem << endl;
        assert (nb_att_rem == nb_att);

//
// Do it once more. This time, it should send an exception
//

        bool except = false;
        try
        {
            device->command_inout("IORemoveAttribute",din);
        }
        catch (Tango::DevFailed&)
        {
            TEST_LOG << "Received exception" << endl;
            except = true;
        }

        assert (except == true);

        TEST_LOG << "   Removing attribute --> OK" << endl;

    }
    catch (Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(-1);
    }


    return 0;

}
