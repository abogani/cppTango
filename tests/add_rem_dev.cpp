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
        device->command_inout("State");
    }
    catch (Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(-1);
    }

    return 0;

}
