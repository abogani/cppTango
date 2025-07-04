// NOLINTBEGIN(*)

#include "old_common.h"

/*
 * Small utility program to help testing locking features.
 *
 * Possible return code:
 *  -1 : major error
 *   0 : success
 *   1 : Exception API_DeviceLocked
 *   2 : All other exceptions
 */

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
        exit(-1);
    }

    // Try an allowed command on the device

    DeviceData din, dout;
    din << (Tango::DevFloat) 2.0;

    try
    {
        dout = device->command_inout("IOFloat", din);
        Tango::DevFloat val;
        dout >> val;

        if(val != 4.0)
        {
            return -1;
        }
    }
    catch(Tango::DevFailed &e)
    {
        //        Except::print_exception(e);
        if(::strcmp(e.errors[0].reason.in(), API_DeviceLocked) == 0)
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }

    delete device;
    return 0;
}

// NOLINTEND(*)
