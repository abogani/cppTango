#include "common.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if (argc != 2)
    {
        TEST_LOG << "usage: %s device" << endl;
        exit(-1);
    }

    string device_name = argv[1];

    try
    {
        device = new DeviceProxy(device_name);
        TEST_LOG << "new DeviceProxy() returned" << endl;

        device->set_timeout_millis(1000);
    }
        catch (CORBA::Exception &e)
        {
                  Except::print_exception(e);
        exit(1);
        }


    while (1)
    {
        TEST_LOG << "Hit any key :" << endl;
        char bid;
        cin >> bid;

        try
        {
//            device->ping();
//            TEST_LOG << "Ping successfull, device IDL version = ";
//            TEST_LOG << device->get_idl_version() << endl;
            device->command_inout("Status");
            TEST_LOG << "command_inout successfull" << endl;
        }
        catch (Tango::CommunicationFailed &e)
        {
            cerr << "Commnunication Failed exception" << endl;
            Except::print_exception(e);
        }
        catch (Tango::ConnectionFailed &e)
        {
            cerr << "Connection Failed exception" << endl;
            Except::print_exception(e);
        }
        catch (CORBA::Exception &e)
        {
            Except::print_exception(e);
        }
        catch (...)
        {
            TEST_LOG << "Unknown exception" << endl;
        }

    }

    return 0;

}
