#include "common.h"

int main(int argc, char **argv)
{
    AttributeProxy *attr;

    if (argc != 2)
    {
        TEST_LOG << "usage: %s attribute" << endl;
        exit(-1);
    }

    string attr_name = argv[1];

    try
    {
        attr = new AttributeProxy(attr_name);
        TEST_LOG << "new AttributeProxy() returned" << endl;
    }
        catch (CORBA::Exception &e)
        {
                  Except::print_exception(e);
        exit(1);
        }

    attr->get_device_proxy()->set_transparency_reconnection(true);

    while (1)
    {
        TEST_LOG << "Hit any key :" << endl;
        char bid;
        cin >> bid;

        try
        {
            attr->ping();
            TEST_LOG << "Ping successfull" << endl;
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
