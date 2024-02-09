// NOLINTBEGIN(*)

#include "common.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if(argc == 1)
    {
        TEST_LOG << "usage: %s device" << std::endl;
        exit(-1);
    }

    std::string device_name = argv[1];

    try
    {
        device = new DeviceProxy(device_name);
    }
    catch(CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    try
    {
        ApiUtil *au = ApiUtil::instance();

        Tango::cb_sub_model mode = au->get_asynch_cb_sub_model();
        assert(mode == PULL_CALLBACK);

        //
        // Start callback thread
        //

        //        char key;
        //        TEST_LOG << "Hit any key ";
        //        cin >> key;

        au->set_asynch_cb_sub_model(PUSH_CALLBACK);

        //        TEST_LOG << "Hit any key ";
        //        cin >> key;

        au->set_asynch_cb_sub_model(PULL_CALLBACK);

        //        TEST_LOG << "Hit any key ";
        //        cin >> key;

        au->set_asynch_cb_sub_model(PUSH_CALLBACK);

        //        TEST_LOG << "Hit any key ";
        //        cin >> key;

        au->set_asynch_cb_sub_model(PULL_CALLBACK);

        //        TEST_LOG << "Hit any key ";
        //        cin >> key;
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(-1);
    }
    catch(CORBA::Exception &ex)
    {
        Except::print_exception(ex);
        exit(-1);
    }

    delete device;

    return 0;
}

// NOLINTEND(*)
