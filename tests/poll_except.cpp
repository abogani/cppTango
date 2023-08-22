#include "common.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if(argc < 2)
    {
        TEST_LOG << "Usage: %s device_name" << endl;
        exit(-1);
    }

    string device_name(argv[1]);

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

    while(true)
    {
        try
        {
            // Test polling_status

            vector<string> *poll_str;
            poll_str = device->polling_status();

            TEST_LOG << poll_str->size() << " object(s) polled for device" << endl;
            //            TEST_LOG << endl;
            //            for (int i = 0;i < poll_str->size();i++)
            //                TEST_LOG << "Polling status = " << (*poll_str)[i] << endl;
            TEST_LOG << endl;

            delete poll_str;
        }
        catch(Tango::DevFailed &e)
        {
            Except::print_exception(e);
        }

        TEST_LOG << "Hit any key :" << endl;
        char bid;
        cin >> bid;
    }

    delete device;
}
