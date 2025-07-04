// NOLINTBEGIN(*)

#include "old_common.h"

/*
 * Test module for sub device diagnostics in the Tango API.
 */

int main(int argc, char **argv)
{
    DeviceProxy *device;
    DeviceProxy *admin;

    if((argc < 4) || (argc > 5))
    {
        TEST_LOG << "usage: sub_dev <device1> <device2> <device3>" << endl;
        exit(-1);
    }

    string device1_name = argv[1];
    string device2_name = argv[2];
    string device3_name = argv[3];

    // sort device names alphabetically
    vector<string> devices;
    std::transform(device1_name.begin(), device1_name.end(), device1_name.begin(), ::tolower);
    std::transform(device2_name.begin(), device2_name.end(), device2_name.begin(), ::tolower);
    std::transform(device3_name.begin(), device3_name.end(), device3_name.begin(), ::tolower);
    devices.push_back(device1_name);
    devices.push_back(device2_name);
    devices.push_back(device3_name);
    sort(devices.begin(), devices.end());

    try
    {
        // be sure that the device name are lower case letters
        //        std::transform(device1_name.begin(), device1_name.end(),
        //                       device1_name.begin(), ::tolower);

        // connect to device
        device = new DeviceProxy(device1_name);

        // Connect to admin device
        string adm_name = device->adm_name();
        admin = new DeviceProxy(adm_name);
    }
    catch(CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    TEST_LOG << endl << "new DeviceProxy(" << device->name() << ") returned" << endl << endl;

    try
    {
        // restart server to clean all sub device lists

        bool except = false;
        try
        {
            admin->command_inout("RestartServer");
        }
        catch(Tango::DevFailed &e)
        {
            Except::print_exception(e);
            except = true;
        }

        assert(except == false);
        TEST_LOG << "  Server restart to clean sub device lists --> OK" << endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // read attribute to have a sub device in the list

        {
            DeviceAttribute da = device->read_attribute("Sub_device_tst");
            TEST_LOG << da << endl;
            bool att_value;
            da >> att_value;
            assert(att_value == true);
        }

        // check the list of sub devices on the administration device

        {
            DeviceData dd = admin->command_inout("QuerySubDevice");
            vector<string> sub_dev_list;
            dd >> sub_dev_list;

            string result = device1_name + " " + devices[1];
            assert(sub_dev_list.size() == 1);
            assert(sub_dev_list[0] == result);

            TEST_LOG << "  Add sub device in attribute method --> OK" << endl;
        }

        // execute command to add sub devices in the list

        {
            DeviceData dd = device->command_inout("SubDeviceTst");
            TEST_LOG << dd << endl;
            bool cmd_value;
            dd >> cmd_value;
            assert(cmd_value == true);

            // let the external thread some time to do its work!
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // check the list of sub devices on the administration device

        {
            DeviceData dd = admin->command_inout("QuerySubDevice");
            vector<string> sub_dev_list;
            dd >> sub_dev_list;

            assert(sub_dev_list.size() == 3);

            string result = devices[1];
            assert(sub_dev_list[0] == result);
            result = device1_name + " " + devices[1];
            assert(sub_dev_list[1] == result);
            result = device1_name + " " + devices[2];
            assert(sub_dev_list[2] == result);

            TEST_LOG << "  Add sub devices in command method and external thread --> OK" << endl;
        }
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(-1);
    }

    delete admin;
    delete device;
    return 0;
}

// NOLINTEND(*)
